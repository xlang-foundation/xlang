#pragma once

#include "object.h"
#include "list.h"

namespace X
{
namespace Data
{
struct VectorCall
{
	XObj* m_context = nil;
	AST::Func* m_func = nil;
	X::LValue m_lVal = nil;
};
class FuncCalls :
	public Object
{
protected:
	std::vector<VectorCall> m_list;
public:
	FuncCalls()
	{
		m_t = ObjType::FuncCalls;
	}
	inline std::vector<VectorCall>& GetList()
	{
		return m_list;
	}
	virtual bool CalcCallables(Runtime* rt, XObj* pContext,
		std::vector<AST::Scope*>& callables) override
	{
		bool bHave = false;
		for (auto& it : m_list)
		{
			bHave |= it.m_func?
				it.m_func->CalcCallables(rt, it.m_context, callables)
				: false;
		}
		return bHave;
	}
	void Add(XObj* pContext, AST::Func* func, X::LValue lVal)
	{
		m_list.push_back(VectorCall{ pContext ,func,lVal });
	}
	bool SetValue(X::Value& val)
	{
		for (auto& i : m_list)
		{
			if (i.m_lVal)
			{
				*i.m_lVal = val;
			}
		}
		return true;
	}
	virtual bool CallEx(XRuntime* rt, XObj* pContext,
		ARGS& params,
		KWARGS& kwParams,
		X::Value& trailer,
		X::Value& retValue) override
	{
		if (m_list.size() == 1)
		{
			auto& fc = m_list[0];
			return fc.m_func->CallEx(rt,
				fc.m_context,
				params, kwParams, trailer,retValue);
		}
		List* pValueList = new List();
		bool bOK = true;
		for (auto& fc : m_list)
		{
			X::Value v0;
			bool bOK = fc.m_func->CallEx(rt,
				fc.m_context,
				params, kwParams, trailer, v0);
			if (bOK)
			{
				pValueList->Add((Runtime*)rt, v0);
			}
			else
			{
				break;
			}
		}
		if (bOK)
		{
			retValue = X::Value(pValueList);
		}
		else
		{
			delete pValueList;
		}
		return bOK;
	}
	virtual bool Call(XRuntime* rt, XObj* pContext,
		ARGS& params,
		KWARGS& kwParams,
		X::Value& retValue)
	{
		if (m_list.size() == 1)
		{
			auto& fc = m_list[0];
			return fc.m_func->Call(rt,
				fc.m_context,
				params, kwParams, retValue);
		}
		List* pValueList = new List();
		bool bOK = true;
		for (auto& fc : m_list)
		{
			X::Value v0;
			bool bOK = fc.m_func->Call(rt,
				fc.m_context,
				params, kwParams, v0);
			if (bOK)
			{
				pValueList->Add((Runtime*)rt, v0);
			}
			else
			{
				break;
			}
		}
		if (bOK)
		{
			retValue = X::Value(pValueList);
		}
		else
		{
			delete pValueList;
		}
		return bOK;
	}
};
}
}