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

#pragma once
#include "exp.h"
#include "scope.h"
#include "block.h"
#include "var.h"
#include "pair.h"
#include "op.h"
#include <vector>
#include "decor.h"
#include "xlang.h"
#include "def.h"


#define _pack_decor_ 0
namespace X
{
namespace AST
{
class Func :
	public Block
{
protected:
	bool m_PassScopeLayout = false;//in lambda case, will run ScopeLayout multiple time
	//use this to avoid it
	Module* m_myModule = nullptr;
	std::vector<Decorator*> m_decors;
	String m_Name = { nil,0 };
	bool m_NameNeedRelease = false;
	int m_Index = -1;//index for this Var,set by compiling
	int m_IndexOfThis = -1;//for THIS pointer
	bool m_needSetHint = false;
	List* Params = nil;
	std::vector<int> m_IndexofParamList;//same size with input positional param
	Expression* RetType = nil;
	void FindMyModule();
	void SetName(Expression* n)
	{
		Var* vName = dynamic_cast<Var*>(n);
		if (vName)
		{
			m_Name = vName->GetName();
		}
		ReCalcHint(n);
	}
	virtual void SetScope(Scope* p) override
	{
		Expression::SetScope(p);
		if (Params) Params->SetScope(p);
		if (RetType) RetType->SetScope(p);
	}
	virtual void SetParams(List* p)
	{
		if (m_needSetHint)
		{
			if (p)
			{
				auto& list = p->GetList();
				if (list.size() > 0)
				{
					ReCalcHint(list[0]);
					if (list.size() > 1)
					{
						ReCalcHint(list[list.size() - 1]);
					}
				}
			}
		}
		Params = p;
		if (Params)
		{
			//all parameters must set as left value
			auto& list = Params->GetList();
			for (auto& l : list)
			{
				l->SetIsLeftValue(true);
			}
			ReCalcHint(Params);
			Params->SetParent(this);
		}
	}
public:
	Func() :
		Block()
	{
		m_type = ObType::Func;
		m_pMyScope = new Scope();
		m_pMyScope->SetType(ScopeType::Func);
		m_pMyScope->SetExp(this);
	}
	~Func()
	{
		if (Params) delete Params;
		if (RetType) delete RetType;
		if (m_NameNeedRelease)
		{
			delete m_Name.s;
		}
		//m_decors's item deleted from Block
#if 0 
		for (auto* d : m_decors)
		{
			delete d;
		}
#endif
		m_decors.clear();
	}
	FORCE_INLINE Module* GetMyModule()
	{
		if (m_myModule == nullptr)
		{
			FindMyModule();
		}
		return m_myModule;
	}
	FORCE_INLINE Scope* GetScope()
	{
		if (m_scope == nullptr)
		{
			m_scope = FindScope();
		}
		return m_scope;
	}
	virtual void ScopeLayout() override;
	std::string getcode(bool includeHead = false);
	virtual bool ToBytes(XlangRuntime* rt,XObj* pContext,X::XLangStream& stream) override
	{
		std::string code;
		for (auto* decor : m_decors)
		{
			code += decor->GetCode()+"\n";
		}
		code += GetCode();
		//change current scope of stream
		Scope* pOldScope = stream.ScopeSpace().GetCurrentScope();
		stream.ScopeSpace().SetCurrentScope(m_pMyScope);
		Block::ToBytes(rt,pContext,stream);
#if _pack_decor_
		stream << (int)m_decors.size();
		for (auto* decor : m_decors)
		{
			SaveToStream(rt, pContext,decor, stream);
		}
#endif
		SaveToStream(rt, pContext, Params, stream);
		SaveToStream(rt, pContext, RetType, stream);
		//restore old scope
		stream.ScopeSpace().SetCurrentScope(pOldScope);

		//Coding itself
		stream << m_Name.size;
		if (m_Name.size > 0)
		{
			stream.append(m_Name.s, m_Name.size);
		}
		stream << (int)m_IndexofParamList.size();
		for (auto idx : m_IndexofParamList)
		{
			stream << idx;
		}
		stream << m_Index << m_IndexOfThis<< m_needSetHint;

		bool bHaveScope = (m_pMyScope != nullptr);	
		stream << bHaveScope;
		if (m_pMyScope)
		{
			m_pMyScope->ToBytes(rt, pContext, stream);
		}
		return true;
	}
	virtual bool FromBytes(X::XLangStream& stream) override
	{
		Block::FromBytes(stream);
#if _pack_decor_
		int size = 0;
		stream >> size;
		for (int i = 0; i < size; i++)
		{
			auto* decor = BuildFromStream<Decorator>(stream);
			if (decor)
			{
				m_decors.push_back(decor);
			}
		}
#endif
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
		stream >> m_Index >> m_IndexOfThis>> m_needSetHint;
		bool bHaveScope = false;
		stream >> bHaveScope;
		if (bHaveScope)
		{
			m_pMyScope->FromBytes(stream);
		}
		return true;
	}
	virtual std::string GetDoc()
	{
		return "";
	}
	FORCE_INLINE void AddDecor(Decorator* pDecor)
	{
		m_decors.push_back(pDecor);
	}
	void NeedSetHint(bool b) { m_needSetHint = b; }
	FORCE_INLINE String& GetNameStr() { return m_Name; }
	FORCE_INLINE std::string GetNameString()
	{
		return std::string(m_Name.s, m_Name.size);
	}
	FORCE_INLINE X::Value GetParameterNameList()
	{
		X::List names;
		if (Params == nullptr)
		{
			return names;
		}
		auto& list = Params->GetList();
		for (auto& l : list)
		{
			if (l->m_type == AST::ObType::Var)
			{
				Var* pVar = dynamic_cast<Var*>(l);
				if (pVar)
				{
					names += pVar->GetNameString();
				}
			}
		}
		return names;
	}
	virtual std::string GetFuncName()
	{
		return GetNameString();
	}
	virtual bool CalcCallables(XlangRuntime* rt, XObj* pContext,
		std::vector<Scope*>& callables) override
	{
		if (Params)
		{
			Params->CalcCallables(rt,pContext,callables);
		}
		callables.push_back(m_pMyScope);
		return true;
	}

	virtual void SetR(Expression* r) override
	{
		if (r->m_type == AST::ObType::Pair)
		{//have to be a pair
			AST::PairOp* pair =dynamic_cast<AST::PairOp*>(r);
			AST::Expression* paramList = pair->GetR();
			if (paramList)
			{
				if (paramList->m_type != AST::ObType::List)
				{
					AST::List* list = new AST::List(paramList);
					SetParams(list);
				}
				else
				{
					SetParams(dynamic_cast<AST::List*>(paramList));
				}
			}
			pair->SetR(nil);//clear R, because it used by SetParams
			AST::Expression* l = pair->GetL();
			if (l)
			{
				SetName(l);
				pair->SetL(nil);
			}
			//content used by func,and clear to nil,
			//not be used anymore, so delete it
			delete pair;
		}
		else if (r->m_type == AST::ObType::Var)
		{
			//for case no () after the name like class and func
			SetName(r);
		}
		//only accept once
		NeedParam = false;

	}

	void SetRetType(Expression* p)
	{
		RetType = p;
		if (RetType)
		{
			RetType->SetParent(this);
		}
	}
	FORCE_INLINE Expression* GetRetType()
	{
		return RetType;
	}
	FORCE_INLINE Expression* GetParams()
	{
		return Params;
	}
	void ChangeStatmentsIntoTranslateMode(
		bool changeIfStatment,
		bool changeLoopStatment);
	virtual bool Call(XRuntime* rt, XObj* pContext,
		ARGS& params,
		KWARGS& kwParams,
		Value& retValue);
	virtual bool CallEx(XRuntime* rt, XObj* pContext,
		ARGS& params,
		KWARGS& kwParams,
		X::Value& trailer,
		X::Value& retValue);
	virtual bool Exec(XlangRuntime* rt,ExecAction& action, XObj* pContext,
		Value& v, LValue* lValue = nullptr) override;
};
class ExternFunc
	:public Func
{
	std::string m_funcName;
	std::string m_doc;
	U_FUNC m_func;
	U_FUNC_EX m_func_ex;
	XObj* m_pContext = nullptr;
public:
	ExternFunc()
	{

	}
	ExternFunc(std::string& funcName, const char* doc,
		U_FUNC& func, XObj* pContext = nullptr)
	{
		m_funcName = funcName;
		if (doc)
		{
			m_doc = doc;
		}
		m_func = func;
		m_type = ObType::BuiltinFunc;
		m_pContext = pContext;
		if (m_pContext)
		{
			m_pContext->IncRef();
		}
	}
	ExternFunc(std::string& funcName, U_FUNC_EX& func, XObj* pContext = nullptr)
	{
		m_funcName = funcName;
		m_func_ex = func;
		m_type = ObType::BuiltinFunc;
		m_pContext = pContext;
		if (m_pContext)
		{
			m_pContext->IncRef();
		}
	}
	~ExternFunc()
	{
		if (m_pContext)
		{
			m_pContext->DecRef();
		}
	}
	FORCE_INLINE virtual std::string GetFuncName() override
	{
		return m_funcName;
	}
	virtual std::string GetDoc() override
	{
		return m_doc;
	}
	virtual bool ToBytes(XlangRuntime* rt, XObj* pContext, X::XLangStream& stream) override
	{
		Expression::ToBytes(rt, pContext, stream);
		stream << m_funcName;
		return true;
	}
	virtual bool FromBytes(X::XLangStream& stream) override
	{
		Expression::FromBytes(stream);
		stream >> m_funcName;
		return true;
	}
	FORCE_INLINE virtual bool CallEx(XRuntime* rt, XObj* pContext,
		ARGS& params,
		KWARGS& kwParams,
		X::Value& trailer,
		X::Value& retValue) override
	{
		if (m_func_ex)
		{
			return m_func_ex(rt, m_pContext,
				pContext == nullptr ? m_pContext : pContext, params,
				kwParams, trailer, retValue);
		}
		else if (m_func)
		{
			return m_func(rt, m_pContext,
				pContext == nullptr ? m_pContext : pContext,
				params, kwParams, retValue);
		}
		else
		{
			return false;
		}
	}
	XObj* GetRightContextForClass(XObj* pContext);
	FORCE_INLINE virtual bool Call(XRuntime* rt, XObj* pContext,
		ARGS& params,
		KWARGS& kwParams,
		Value& retValue) override
	{
		//TODO:Shawn 7/27/2023, need to double check here
		// for class inherit from Native Package, the pContext will point XClassObject
		//but for func call, m_pContext should be Package
		XObj* pPassInContext;
		if (pContext == nullptr)
		{
			pPassInContext = m_pContext;
		}
		else if (pContext->GetType() == ObjType::XClassObject 
			&& m_pContext && m_pContext->GetType() == ObjType::Package)
		{
			pPassInContext = GetRightContextForClass(pContext);
		}
		else
		{
			pPassInContext = pContext;
		}
		X::Value trailer;
		if ((!m_func)&& params.size() > 0)
		{
			trailer = params[params.size() - 1];
		}
		return m_func ? m_func(rt, pPassInContext, pContext,
			params, kwParams, retValue) : 
			(
				m_func_ex ? m_func_ex(rt, m_pContext,
					pContext == nullptr ? m_pContext : pContext, params,
					kwParams, trailer,retValue) : false
			);
	}
};

}
}