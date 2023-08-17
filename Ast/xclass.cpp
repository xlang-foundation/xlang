#include "xclass.h"
#include "object.h"
#include "function.h"
#include "event.h"
#include "xclass_object.h"

namespace X
{
namespace AST
{
bool XClass::Call(XlangRuntime* rt,
	XObj* pContext,
	ARGS& params, 
	KWARGS& kwParams,
	Value& retValue)
{
	Data::XClassObject* obj = new Data::XClassObject(this);
	obj->IncRef();
	if (m_constructor)
	{
		m_constructor->Call(rt,obj,params, kwParams,retValue);
	}
	retValue = Value(obj);
	obj->DecRef();
	return true;
}
void XClass::ScopeLayout()
{
	Scope* pMyScope = GetScope();
	if (pMyScope)
	{
		std::string strName(m_Name.s, m_Name.size);
		m_Index = pMyScope->AddOrGet(strName, false);
	}
	if (Params)
	{
		auto& list = Params->GetList();
		for (auto i : list)
		{
			//for class, the parameter must be there, not left value
			i->SetIsLeftValue(false);
			i->ScopeLayout();
		}
	}
}

XClass* XClass::FindBase(XlangRuntime* rt, std::string& strName)
{
	XClass* pBase = nullptr;
	auto* pScope = GetScope();
	while (pScope != nullptr)
	{
		int idx = pScope->AddOrGet(strName, true);
		if (idx != -1)
		{
			Value v0;
			if (pScope->Get(rt,nullptr,idx, v0))
			{
				if (v0.IsObject())
				{
					Data::Object* pBaseObj = dynamic_cast<Data::Object*>(v0.GetObj());
					auto* pXBaseObj = dynamic_cast<Data::XClassObject*>(pBaseObj);
					if (pXBaseObj)
					{
						pBase = pXBaseObj->GetClassObj();
						break;
					}
				}
			}
		}
		pScope = pScope->GetParentScope();
	}
	return pBase;
}
int XClass::AddOrGet(std::string& name, bool bGetOnly, Scope** ppRightScope)
{
	int retIdx = Scope::AddOrGet(name, bGetOnly, ppRightScope);
	if (retIdx >= 0)
	{
		return retIdx;
	}
	for (auto& v : m_bases)
	{
		if (v.IsObject())
		{
			Data::Object* pRealObj = dynamic_cast<Data::Object*>(v.GetObj());
			std::vector<AST::Scope*> bases;
			pRealObj->GetBaseScopes(bases);
			for (auto* pScope : bases)
			{
				Scope* pRigthScope = nullptr;
				int idx = pScope->AddOrGet(name, bGetOnly, &pRigthScope);
				if (idx >= 0)
				{
					*ppRightScope = (pRigthScope == nullptr)? pScope:pRigthScope;
					return idx;
				}
			}
		}
	}
	return -1;
}
bool XClass::Set(XlangRuntime* rt, XObj* pContext, int idx, Value& v)
{
	bool bSet = false;
	if (pContext)
	{
		Data::XClassObject* pObj = dynamic_cast<Data::XClassObject*>(pContext);
		if (pObj)
		{
			pObj->GetStack()->Set(idx, v);
			bSet = true;
		}
	}
	if(!bSet && m_stackFrame)
	{
		m_stackFrame->Set(idx, v);
		bSet = true;
	}
	return bSet;
}
bool XClass::Get(XlangRuntime* rt,XObj* pContext, int idx, Value& v, LValue* lValue)
{
	if (pContext)
	{
		Data::XClassObject* pObj = dynamic_cast<Data::XClassObject*>(pContext);
		//TODO: need to very here 
		//7/26/2023,  we change here to avoid class inheritance can find
		//right value
		//only when the passed pContext is same AST Class, then we do use instance of this
		//class's stack
		if (pObj->GetClassObj() == this)
		{
			pObj->GetStack()->Get(idx, v, lValue);
			return true;
		}
	}
	if (m_stackFrame)
	{
		m_stackFrame->Get(idx, v, lValue);
	}
	return true;
}
bool XClass::Exec(XlangRuntime* rt,ExecAction& action,XObj* pContext,Value& v, LValue* lValue)
{
	m_stackFrame = new StackFrame(this);
	m_stackFrame->SetVarCount((int)m_Vars.size());
	if (m_Index == -1)
	{
		ScopeLayout();
		if (m_Index == -1)
		{
			return false;
		}
	}
	int nConstructorIndex = QueryConstructor();
	Block::Exec(rt, action, pContext, v, lValue);
	if (nConstructorIndex >= 0)
	{
		X::Value varFunc;
		m_stackFrame->Get(nConstructorIndex, varFunc);
		if (varFunc.IsObject())
		{
			Data::Function* pFuncObj = dynamic_cast<Data::Function*>(varFunc.GetObj());
			if (pFuncObj)
			{
				m_constructor = pFuncObj->GetFunc();
			}
		}
	}
	//then check back constructor,destructor
	Data::XClassObject* cls = new Data::XClassObject(this);
	Value v0(cls);
	bool bOK = m_scope->Set(rt,pContext,m_Index, v0);
	v = v0;

	//prcoess base classes
	if (Params)
	{
		auto& list = Params->GetList();
		for (auto i : list)
		{
			Value paramObj;
			ExecAction action0;
			bOK = i->Exec(rt, action0, pContext, paramObj);
			if (paramObj.IsObject())
			{
				auto ty = paramObj.GetObj()->GetType();
				if (ty == ObjType::Function || ty == ObjType::FuncCalls)
				{
					X::ARGS args(0);
					X::KWARGS kwargs;
					X::Value paramRealObj;
					if (paramObj.GetObj()->Call(rt, nullptr, args, kwargs, paramRealObj))
					{
						paramObj = paramRealObj;
					}
				}
			}
			m_bases.push_back(paramObj);
		}
	}
	return bOK;
}
void XClass::Add(Expression* item)
{
	//deal with class prop, for case just put prop name here, not assign
	if (item->m_type == ObType::Var)
	{
		item->SetIsLeftValue(true);//use this way to add this prop into class's scope
	}
	else if (item->m_type == ObType::Param)
	{
		Param* param = dynamic_cast<Param*>(item);
		if (param->GetName())
		{
			param->GetName()->SetIsLeftValue(true);
		}
	}
	else if (item->m_type == ObType::Assign)
	{
		Assign* assign = dynamic_cast<Assign*>(item);
		if (assign->GetL())
		{
			auto* l = assign->GetL();
			if (l->m_type == ObType::Var)
			{
				l->SetIsLeftValue(true);
			}
			else if (l->m_type == ObType::Param)
			{
				Param* param = dynamic_cast<Param*>(l);
				if (param->GetName())
				{
					param->GetName()->SetIsLeftValue(true);
				}
			}
		}
	}
	Block::Add(item);
}
}
}