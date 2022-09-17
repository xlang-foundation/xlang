#pragma once
#include "exp.h"
#include "scope.h"
#include "block.h"
#include "var.h"
#include "pair.h"
#include "op.h"
#include <vector>
#include "decor.h"

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
	int m_Index = -1;//index for this Var,set by compiling
	int m_positionParamCnt = 0;
	int m_paramStartIndex = 0;
	int m_IndexOfThis = -1;//for THIS pointer
	bool m_needSetHint = false;
	Expression* Name = nil;
	List* Params = nil;
	Expression* RetType = nil;
	void SetName(Expression* n)
	{
		Var* vName = dynamic_cast<Var*>(n);
		if (vName)
		{
			m_Name = vName->GetName();
		}
		Name = n;
		if (Name)
		{
			Name->SetParent(this);
		}
	}

	virtual void ScopeLayout() override;
	virtual void SetParams(List* p)
	{
		if (m_needSetHint)
		{
			if (p)
			{
				auto& list = p->GetList();
				if (list.size() > 0)
				{
					auto exp = list[0];
					SetHint(exp->GetStartLine(), exp->GetEndLine(),
						exp->GetCharPos());
				}
			}
		}
		Params = p;
		if (Params)
		{
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
	}
	inline void AddDecor(Decorator* pDecor)
	{
		m_decors.push_back(pDecor);
	}
	void NeedSetHint(bool b) { m_needSetHint = b; }
	Expression* GetName() { return Name; }
	String& GetNameStr() { return m_Name; }
	virtual std::string GetNameString() override
	{
		return std::string(m_Name.s, m_Name.size);
	}
	virtual bool CalcCallables(Runtime* rt, XObj* pContext,
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
	virtual bool Run(Runtime* rt, XObj* pContext,
		Value& v, LValue* lValue = nullptr) override;
};
class ExternFunc
	:virtual public Func
{
	std::string m_funcName;
	U_FUNC m_func;
public:
	ExternFunc(std::string& funcName, U_FUNC func)
	{
		m_funcName = funcName;
		m_func = func;
		m_type = ObType::BuiltinFunc;
	}
	inline virtual bool Call(XRuntime* rt, XObj* pContext,
		std::vector<Value>& params,
		KWARGS& kwParams,
		Value& retValue) override
	{
		return m_func ? m_func(rt,
			pContext, params, kwParams, retValue) : false;
	}
};

}
}