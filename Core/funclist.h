#pragma once

#include "object.h"
#include "list.h"

namespace X
{
namespace Data
{
struct VectorCall
{
	void* m_context = nil;
	AST::Func* m_func = nil;
	AST::LValue m_lVal = nil;
};
class FuncCalls :
	public Object
{
protected:
	std::vector<VectorCall> m_list;
public:
	FuncCalls()
	{
		m_t = Type::FuncCalls;
	}
	inline std::vector<VectorCall>& GetList()
	{
		return m_list;
	}
	void Add(void* pContext, AST::Func* func, AST::LValue lVal)
	{
		m_list.push_back(VectorCall{ pContext ,func,lVal });
	}
	bool SetValue(AST::Value& val)
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
	virtual bool Call(Runtime* rt, std::vector<AST::Value>& params,
		std::unordered_map<std::string, AST::Value>& kwParams,
		AST::Value& retValue)
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
			AST::Value v0;
			bool bOK = fc.m_func->Call(rt,
				fc.m_context,
				params, kwParams, v0);
			if (bOK)
			{
				pValueList->Add(rt, v0);
			}
			else
			{
				break;
			}
		}
		if (bOK)
		{
			retValue = AST::Value(pValueList);
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