#include "var.h"
#include "value.h"
#include "object.h"
#include "function.h"
#include "prop.h"
#include <iostream>
#include "module.h"
#include "expr_scope.h"

namespace X
{
namespace AST
{
	void Var::EncodeExtern(XlangRuntime* rt, XObj* pContext, X::XLangStream& stream)
	{
		Value v0;
		ExecAction action;
		if (!ExpExec(this,rt,action, pContext, v0))
		{
			return;
		}
		ScopeType st = m_scope->GetType();
		Scope* thisScope = m_scope;
		//code scope ID for each Var
		//if this scope is a top module, code the scopeid as 0,
		//then in decoding stage, will use top module to replace it
		if (st == ScopeType::Module)
		{
			thisScope = nullptr;
		}
		stream << st;
		stream << (unsigned long long)(void*)thisScope;
		unsigned long long id = 0;
		bool isExternFunc = false;
		if (v0.IsObject())
		{
			XObj* pObj = v0.GetObj();
			if (pObj->GetType() == ObjType::Function)
			{
				auto* pFuncObj = dynamic_cast<Data::Function*>(pObj);
				if (pFuncObj)
				{
					auto* pFunc = pFuncObj->GetFunc();
					if (pFunc->m_type == ObType::BuiltinFunc)
					{
						isExternFunc = true;
					}
				}
			}
			if (!isExternFunc)
			{
				id = (unsigned long long)pObj;
			}
		}
		stream << id;// decoding stage, if Zero, treat as value not object
		stream << isExternFunc;
		if (id ==0)
		{
			if (!isExternFunc)
			{
				stream << v0;
			}
		}
		else
		{
			XObj* pObj = v0.GetObj();
			auto* addr = stream.ScopeSpace().Query(id);
			if (addr == nullptr)
			{//not found, dump this object and add into ScopeSpace
				stream << true;//means:object dump here
				stream << v0;
				stream.ScopeSpace().Add(id, pObj);
			}
			else
			{
				stream << false;//no object here, need to use ID to lookup from ScopeSpace
			}
		}
	}
	void Var::DecodeExtern(XlangRuntime* rt, XObj* pContext, X::XLangStream& stream)
	{
		ScopeType st;
		stream >> st;
		unsigned long long scopeId = 0;
		stream >> scopeId;
		Scope* pCurScope = nullptr;
		void* pScopeAddr = stream.ScopeSpace().Query(scopeId);
		if (pScopeAddr == nullptr)
		{
			switch (st)
			{
			case ScopeType::Module:
			{
				if (scopeId == 0)
				{
					if (rt)
					{
						pCurScope = rt->M()->GetMyScope();
					}
				}
				else
				{//todo: need to do more to recove sub-module
					AST::Module* pModule = new AST::Module();
					pModule->ScopeLayout();
					pCurScope = pModule->GetMyScope();
				}
			}
				break;
			case ScopeType::Class:
			{
				//TODO: how to deal with the pClass Pointer
				auto* pClass = new AST::XClass();
				pCurScope = pClass->GetMyScope();
			}
				break;
			case ScopeType::Func:
			{
				//TODO: How to deal with the pFunc Pointer
				auto* pFunc = new AST::Func();
				pCurScope = pFunc->GetMyScope();
			}
				break;
			default:
				break;
			}
			if (scopeId != 0)
			{
				stream.ScopeSpace().Add(scopeId, pCurScope);
			}
		}
		else
		{
			pCurScope = (Scope*)pScopeAddr;
		}
		unsigned long long objid = 0;
		stream >> objid;
		bool isExternFunc = false;
		stream >> isExternFunc;
		std::string varName = GetNameString();
		m_scope = pCurScope;
		Value v0;
		if (objid == 0)
		{
			if (isExternFunc)
			{
				ScopeLayout();//find the index
			}
			else
			{
				stream >> v0;
				if (pCurScope)
				{
					//will modify Index to match with new top module
					SCOPE_FAST_CALL_AddOrGet0_NoDef(Index,pCurScope,varName, false);
					rt->Set(pCurScope,pContext, Index, v0);
				}
			}
		}
		else
		{
			bool bObjEmbedHere = false;
			stream >> bObjEmbedHere;
			if (bObjEmbedHere)
			{
				stream >> v0;
				if (pCurScope)
				{
					SCOPE_FAST_CALL_AddOrGet0_NoDef(Index,pCurScope,varName, false);
					rt->Set(pCurScope, pContext, Index, v0);
				}
			}
			else
			{
				//find the index to match the new envirment
				ScopeLayout();
			}
		}
		m_scope = pCurScope;
	}
	bool Var::ToBytes(XlangRuntime* rt, XObj* pContext, X::XLangStream& stream)
	{
		Expression::ToBytes(rt, pContext, stream);
		stream << Name.size;
		if (Name.size > 0)
		{
			stream.append(Name.s, Name.size);
		}
		if (Index == -1 || m_scope == nullptr)
		{
			ScopeLayout();
		}
		stream << Index;
		//if Index is still -1, this case
		//will happen in decor function
		//just treat as local var
		bool isExtern = (Index != -1) 
#if __TODO_SCOPE__
			&& (dynamic_cast<X::Data::ExpressionScope*>(m_scope) == nullptr)
#endif
			&& (m_scope != stream.ScopeSpace().GetCurrentScope())
			&& (m_scope != stream.ScopeSpace().GetCurrentClassScope());
		stream << isExtern;
		//check the value if it is external
		if (isExtern)
		{
			EncodeExtern(rt, pContext, stream);
		}
		return true;
	}

	bool Var::FromBytes(X::XLangStream& stream)
	{
		Expression::FromBytes(stream);
		stream >> Name.size;
		if (Name.size > 0)
		{
			Name.s = new char[Name.size];
			m_needRelease = true;
			stream.CopyTo(Name.s, Name.size);
		}
		else
		{
			Name.s = nullptr;
		}
		stream >> Index;
		bool isExtern = false;
		stream >> isExtern;
		if (isExtern)
		{
			DecodeExtern(stream.ScopeSpace().RT(),stream.ScopeSpace().Context(), stream);
		}
		return true;
	}
	bool Var::GetPropValue(XlangRuntime* rt, XObj* pContext,XObj* pObj, Value& val)
	{
		bool bOK = false;
		auto* pPropObj = dynamic_cast<Data::PropObject*>(pObj);
		if (pPropObj)
		{
			bOK = pPropObj->GetPropValue(rt, pContext, val);
		}
		return bOK;
	}
bool Var::CalcCallables(XlangRuntime* rt, XObj* pContext,
		std::vector<Scope*>& callables)
{
	Value val;
	ExecAction action;
	bool bOK = ExpExec(this,rt,action, pContext, val);
	if (bOK && val.IsObject())
	{
		Data::Object* pObj = dynamic_cast<Data::Object*>(val.GetObj());
		bOK = pObj->CalcCallables(rt, pContext, callables);
	}
	return bOK;
}
void Var::ScopeLayout(std::vector<AST::Scope*>& candidates)
{
	bool matched = false;
	if (m_scope && Index != -1)
	{//check if matche with one candidate
		for (auto it : candidates)
		{
			if (it == m_scope)
			{
				matched = true;
				break;
			}
		}
	}
	if (matched)
	{
		return;
	}
	std::string strName(Name.s, Name.size);
	for (auto s : candidates)
	{
		Scope* pRightScope = nullptr;
		SCOPE_FAST_CALL_AddOrGet(idx,s,strName, !m_isLeftValue, &pRightScope);
		if (idx != -1)
		{//use the scope to find this name as its scope
			m_scope = (pRightScope == nullptr) ? s : pRightScope;
			Index = idx;
			break;
		}
	}
}

// if this var is inside a Decorator for example: 
// @cantor.Task(NPU=1 and OS =='Windows')
// or inside a function call for example: test_func2.run(i,20,TaskTag=i)
// this var can't be left Value, because we used it as
// expresion
// so here we check if it is inside a Pair
//TODO(Shawn) 6/15/2023: check lambda function with { } some var define, if it is still correct

void Var::ScopeLayout()
{
	Scope* pMyScope = GetScope();
	int idx = -1;
	if (m_isLeftValue)
	{
		bool IsInsidePair = false;
		auto pa = m_parent;
		while (pa != nullptr)
		{
			if (pa->m_type == ObType::Pair)
			{
				IsInsidePair = true;
				break;
			}
			pa = pa->GetParent();
		}
		if (IsInsidePair)
		{
			m_isLeftValue = false;
		}
	}
	bool bIsLeftValue = m_isLeftValue;

	Expression* pFromExp = this;
	while (pMyScope != nullptr && idx <0)
	{
		std::string strName(Name.s, Name.size);
		Scope* pRightScope = nullptr;
		SCOPE_FAST_CALL_AddOrGet_NoDef(idx,pMyScope,strName,!m_isLeftValue,&pRightScope);
		if (idx == (int)ScopeVarIndex::EXTERN)
		{//for extern var, always looking up parent scopes
			bIsLeftValue = false;
		}
		if (bIsLeftValue)
		{//Left value will add into local scope
		//don't need to go up
			break;
		}
		if (idx >=0)
		{//use the scope to find this name as its scope
			m_scope = (pRightScope==nullptr)?pMyScope: pRightScope;
			break;
		}
		//Find next upper real scope
		pMyScope = nullptr;
		Expression* pa = pFromExp->GetParent();
		while (pa != nullptr)
		{
			pMyScope = pa->GetMyScope();
			if (pMyScope)
			{
				//save for next loop
				pFromExp = pa;
				break;
			}
			pa = pa->GetParent();
		}
	}
	Index = idx;
}
}
}