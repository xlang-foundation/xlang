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
		pObj->GetStack()->Get(idx, v, lValue);
	}
	else if (m_stackFrame)
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
	Block::Exec(rt, action, pContext, v, lValue);
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
			std::string strVarName;
			if (i->m_type == ObType::Var)
			{
				Var* varName = dynamic_cast<Var*>(i);
				String& szName = varName->GetName();
				strVarName = std::string(szName.s, szName.size);
				auto* pBaseClass = FindBase(rt,strVarName);
				if (pBaseClass)
				{
					m_bases.push_back(pBaseClass);
				}
			}
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