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

#include "future.h"
#include "scope.h"
#include "function.h"
#include "glob.h"
#include "obj_func_scope.h"

namespace X
{
	namespace Data
	{
		static Obj_Func_Scope<2> _futureScope;

		void Future::GetBaseScopes(std::vector<AST::Scope*>& bases)
		{
			Object::GetBaseScopes(bases);
			bases.push_back(_futureScope.GetMyScope());
		}
		void Future::Init()
		{
			_futureScope.Init();
			{
				auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					X::ARGS& params,
					X::KWARGS& kwParams,
					X::Value& retValue)
					{
						bool bOK = false;
						if (params.size() > 0)
						{
							Future* pObj = dynamic_cast<Future*>(pContext);
							bOK = pObj->GetResult(retValue, params[0]);
						}
						else
						{
							retValue = Value(bOK);
						}
						return bOK;
					};
				_futureScope.AddFunc("get", "get(timeout)", f);
			}
			{
				auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					X::ARGS& params,
					X::KWARGS& kwParams,
					X::Value& retValue)
					{
						if (params.size() > 0)
						{
							Future* pObj = dynamic_cast<Future*>(pContext);
							pObj->SetThenProc(params[0]);
						}
						//still return this Future Object to setup chain calls
						retValue = X::Value(pContext);
						return true;
					};
				_futureScope.AddFunc("then", "then(proc)", f);
			}
		}
		void Future::cleanup()
		{
			_futureScope.Clean();
		}
		void Future::SetVal(X::Value& v)
		{
			std::vector<X::Value> thenProcs;
			Lock();
			m_GotVal = true;
			m_Val = v;
			thenProcs = m_thenProcs;
			for (auto* w : m_waits)
			{
				w->Release();
			}
			Unlock();
			for(auto& proc: thenProcs)
			{
				if (!proc.IsObject())
				{
					continue;
				}
				//TODO: which name we should use?
				std::string name("future");
				XlangRuntime* rt = X::G::I().Threading(name,nullptr);
				X::ARGS params(1);
				X::KWARGS kwargs;
				X::Value retValue;
				params.push_back(v);
				proc.GetObj()->Call(rt, this, params, kwargs, retValue);
			}
		}
		bool Future::GetResult(X::Value& retVal,int timeout)
		{
			bool bOK = false;
			Lock();
			if (m_GotVal)
			{
				retVal = m_Val;
				bOK = true;
			}
			Unlock();
			if (bOK)
			{
				return true;
			}
			XWait* pWait = new XWait();
			Lock();
			m_waits.push_back(pWait);
			Unlock();
			bOK = pWait->Wait(timeout);
			Lock();
			if (m_GotVal)
			{
				retVal = m_Val;
				bOK = true;
			}
			for (auto it = m_waits.begin(); it != m_waits.end();)
			{
				if (*it == pWait)
				{
					it = m_waits.erase(it);
					break;
				}
				else
				{
					++it;
				}
			}
			Unlock();
			delete pWait;
			return bOK;
		}
	}
}