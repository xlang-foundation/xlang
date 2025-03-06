/*
Copyright (C) 2024 The XLang Foundation
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

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
	BuildBaseInstances(rt, obj);
	obj->IncRef();
	if (m_constructor)
	{
		m_constructor->Call(rt,obj,params, kwParams,retValue);
	}
	retValue = Value(obj);
	obj->DecRef();
	return true;
}
bool XClass::FromBytes(X::XLangStream& stream)
{
	Block::FromBytes(stream);
	Params = BuildFromStream<List>(stream);
	if (Params)
	{
		Params->SetParent(this);
	}
	RetType = BuildFromStream<Expression>(stream);
	if (RetType)
	{
		RetType->SetParent(this);
	}

	//decoding itself
	stream >> m_Name.size;
	if (m_Name.size > 0)
	{
		m_Name.s = new char[m_Name.size];
		stream.CopyTo(m_Name.s, m_Name.size);
		m_NameNeedRelease = true;
	}
	int paramCnt = 0;
	stream >> paramCnt;
	for (int i = 0; i < paramCnt; i++)
	{
		int idx;
		stream >> idx;
		m_IndexofParamList.push_back(idx);
	}
	stream >> m_Index >> m_IndexOfThis >> m_needSetHint;
	m_pMyScope->FromBytes(stream);
	m_variableFrame = new StackFrame(m_pMyScope);
	m_pMyScope->SetVarFrame(m_variableFrame);
	m_variableFrame->FromBytes(stream);
#if _TODO_
	ScopeLayout();
	XObj* pContext = stream.ScopeSpace().Context();
	X::Data::XClassObject* pClassObj = dynamic_cast<X::Data::XClassObject*>(pContext);
	if (pClassObj)
	{
		pClassObj->AssignClass(this);
	}
	ExecAction  action;
	X::Value retObj;
	Exec_i(stream.ScopeSpace().RT(), action, pContext,retObj);
	if (pClassObj)
	{
		BuildBaseInstances(stream.ScopeSpace().RT(), pClassObj);
	}
#endif
	return true;
}

void XClass::ScopeLayout()
{
	Scope* pMyScope = GetScope();
	if (pMyScope)
	{
		std::string strName(m_Name.s, m_Name.size);
		SCOPE_FAST_CALL_AddOrGet0_NoDef(m_Index,pMyScope,strName, false);
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
	Expression* pFromExp = this;
	while (pScope != nullptr)
	{
		SCOPE_FAST_CALL_AddOrGet0(idx,pScope,strName, true);
		if (idx != -1)
		{
			Value v0;
			if (rt->Get(pScope,nullptr,idx, v0))
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
		//Find next upper real scope
		pScope = nullptr;
		Expression* pa = pFromExp->GetParent();
		while (pa != nullptr)
		{
			pScope = pa->GetMyScope();
			if (pScope)
			{
				//save for next loop
				pFromExp = pa;
				break;
			}
			pa = pa->GetParent();
		}
	}
	return pBase;
}
int XClass::AddOrGet(std::string& name, bool bGetOnly, Scope** ppRightScope)
{
	SCOPE_FAST_CALL_AddOrGet(retIdx,m_pMyScope,name, bGetOnly, ppRightScope);
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
				SCOPE_FAST_CALL_AddOrGet(idx,pScope,name, bGetOnly, &pRigthScope);
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
	if(!bSet && m_variableFrame)
	{
		m_variableFrame->Set(idx, v);
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
	if (m_variableFrame)
	{
		m_variableFrame->Get(idx, v, lValue);
	}
	return true;
}
bool XClass::Exec(XlangRuntime* rt, ExecAction& action, XObj* pContext, Value& v, LValue* lValue)
{
	bool bOK = Exec_i(rt, action, pContext, v, lValue);
	Data::XClassObject* cls = new Data::XClassObject(this);
	Value v0(cls);
	if (m_scope)
	{
		bOK = rt->Set(m_scope,pContext, m_Index, v0);
	}
	v = v0;
	return bOK;
}

bool XClass::Exec_i(XlangRuntime* rt, ExecAction& action, XObj* pContext, Value& v, LValue* lValue)
{
	if (m_Index == -1 || m_scope == nullptr)
	{
		ScopeLayout();
		if (m_Index == -1)
		{
			return false;
		}
	}
	//then check back constructor,destructor
	int nConstructorIndex = QueryConstructor();
	Block::Exec_i(rt, action, pContext, v, lValue);
	if (nConstructorIndex >= 0)
	{
		X::Value varFunc;
		m_variableFrame->Get(nConstructorIndex, varFunc);
		if (varFunc.IsObject())
		{
			Data::Function* pFuncObj = dynamic_cast<Data::Function*>(varFunc.GetObj());
			if (pFuncObj)
			{
				m_constructor = pFuncObj->GetFunc();
			}
		}
	}

	//prcoess base classes
	if (Params)
	{
		auto& list = Params->GetList();
		for (auto i : list)
		{
			Value paramObj;
			ExecAction action0;
			bool bOK = ExpExec(i,rt, action0, pContext, paramObj);
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

	return true;
}
bool XClass::BuildBaseInstances(XlangRuntime* rt,XObj* pClassObj)
{
	if (Params)
	{
		auto& list = Params->GetList();
		auto& bases = (dynamic_cast<X::Data::XClassObject*>(pClassObj))->GetBases();
		for (auto i : list)
		{
			Value paramObj;
			ExecAction action0;
			bool bOK = ExpExec(i,rt, action0, pClassObj, paramObj);
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
			bases.push_back(paramObj);
		}
	}
	return true;
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