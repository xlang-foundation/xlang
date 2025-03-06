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

#include "object.h"
#include "list.h"

namespace X
{
namespace Data
{
struct VectorCall
{
	X::Value contextObj;
	X::Value m_func;
	X::LValue m_lVal = nil;
	X::XObj* GetContext() { return contextObj.GetObj(); }
};
class FuncCalls :
	public Object
{
protected:
	std::vector<VectorCall> m_list;
public:
	FuncCalls():Object(), XObj()
	{
		m_t = ObjType::FuncCalls;
	}
	FORCE_INLINE std::vector<VectorCall>& GetList()
	{
		return m_list;
	}
	virtual bool ToBytes(XlangRuntime* rt, XObj* pContext, X::XLangStream& stream)
	{
		Object::ToBytes(rt, pContext, stream);
		m_lock.Lock();
		stream << (int)m_list.size();
		for (auto& it : m_list)
		{
			stream << it.contextObj;
			stream << it.m_func;
			//todo: m_lVal?
		}
		m_lock.Unlock();
		return true;
	}
	virtual bool FromBytes(X::XLangStream& stream)
	{
		int size = 0;
		stream >> size;
		m_lock.Lock();
		for (int i = 0; i < size; i++)
		{
			VectorCall vc;
			stream >> vc.contextObj;
			stream >> vc.m_func;
			m_list.push_back(vc);
		}
		m_lock.Unlock();
		return true;
	}
	virtual bool CalcCallables(XlangRuntime* rt, XObj* pContext,
		std::vector<AST::Scope*>& callables) override
	{
		bool bHave = false;
		for (auto& it : m_list)
		{
			bool bCallable = false;
			if (it.m_func.IsObject())
			{
				auto* pDataObj = dynamic_cast<Data::Object*>(it.m_func.GetObj());
				bCallable = pDataObj->CalcCallables(rt, it.GetContext(), callables);
			}
			bHave |= bCallable;
		}
		return bHave;
	}
	void Add(XObj* pContext, X::Value& func, X::LValue lVal)
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
			return fc.m_func.GetObj()->CallEx(rt,
				fc.GetContext(),
				params, kwParams, trailer,retValue);
		}
		List* pValueList = new List();
		bool bOK = true;
		for (auto& fc : m_list)
		{
			X::Value v0;
			bool bOK = fc.m_func.GetObj()->CallEx(rt,
				fc.GetContext(),
				params, kwParams, trailer, v0);
			if (bOK)
			{
				pValueList->Add((XlangRuntime*)rt, v0);
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
			return fc.m_func.GetObj()->Call(rt,
				fc.GetContext(),
				params, kwParams, retValue);
		}
		List* pValueList = new List();
		bool bOK = true;
		for (auto& fc : m_list)
		{
			X::Value v0;
			bool bOK = fc.m_func.GetObj()->Call(rt,
				fc.GetContext(),
				params, kwParams, v0);
			if (bOK)
			{
				pValueList->Add((XlangRuntime*)rt, v0);
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