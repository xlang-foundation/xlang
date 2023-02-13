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
	virtual public Block,
	virtual public Scope
{
protected:
	std::vector<Decorator*> m_decors;
	String m_Name = { nil,0 };
	bool m_NameNeedRelease = false;
	int m_Index = -1;//index for this Var,set by compiling
	int m_IndexOfThis = -1;//for THIS pointer
	bool m_needSetHint = false;
	List* Params = nil;
	std::vector<int> m_IndexofParamList;//same size with input positional param
	Expression* RetType = nil;
	void SetName(Expression* n)
	{
		Var* vName = dynamic_cast<Var*>(n);
		if (vName)
		{
			m_Name = vName->GetName();
		}
		ReCalcHint(n);
	}
	virtual void ScopeLayout() override;
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
			ReCalcHint(Params);
			Params->SetParent(this);
		}
	}
public:
	Func() :
		Block(), UnaryOp(), Operator(),
		Scope(),ObjRef()
	{
		m_type = ObType::Func;
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
		stream.ScopeSpace().SetCurrentScope(dynamic_cast<Scope*>(this));
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
		Scope::ToBytes(rt, pContext, stream);
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
		Scope::FromBytes(stream);
		return true;
	}
	virtual std::string GetDoc()
	{
		return "";
	}
	inline void AddDecor(Decorator* pDecor)
	{
		m_decors.push_back(pDecor);
	}
	void NeedSetHint(bool b) { m_needSetHint = b; }
	String& GetNameStr() { return m_Name; }
	virtual std::string GetNameString() override
	{
		return std::string(m_Name.s, m_Name.size);
	}
	virtual bool CalcCallables(XlangRuntime* rt, XObj* pContext,
		std::vector<Scope*>& callables) override
	{
		if (Params)
		{
			Params->CalcCallables(rt,pContext,callables);
		}
		callables.push_back(this);
		return true;
	}
	virtual Scope* GetParentScope() override
	{
		return FindScope();
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
	}

	void SetRetType(Expression* p)
	{
		RetType = p;
		if (RetType)
		{
			RetType->SetParent(this);
		}
	}
	virtual int AddOrGet(std::string& name, bool bGetOnly)
	{
		int retIdx = Scope::AddOrGet(name, bGetOnly);
		return retIdx;
	}
	virtual bool Call(XRuntime* rt, XObj* pContext,
		std::vector<Value>& params,
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
	:virtual public Func
{
	std::string m_funcName;
	std::string m_doc;
	U_FUNC m_func =nullptr;
	U_FUNC_EX m_func_ex = nullptr;
	XObj* m_pContext = nullptr;
public:
	ExternFunc()
	{

	}
	ExternFunc(std::string& funcName, const char* doc,
		U_FUNC func, XObj* pContext = nullptr)
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
	ExternFunc(std::string& funcName, U_FUNC_EX func, XObj* pContext = nullptr)
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
	inline virtual bool CallEx(XRuntime* rt, XObj* pContext,
		ARGS& params,
		KWARGS& kwParams,
		X::Value& trailer,
		X::Value& retValue) override
	{
		return m_func_ex ? m_func_ex(rt,
			pContext == nullptr ? m_pContext : pContext, params, 
			kwParams, trailer,retValue) : false;
	}
	inline virtual bool Call(XRuntime* rt, XObj* pContext,
		std::vector<Value>& params,
		KWARGS& kwParams,
		Value& retValue) override
	{
		return m_func ? m_func(rt,
			pContext==nullptr? m_pContext: pContext, 
			params, kwParams, retValue) : 
			(
				m_func_ex ? m_func_ex(rt,
					pContext == nullptr ? m_pContext : pContext, params,
					kwParams, params[0], retValue) : false
			);
	}
};

}
}