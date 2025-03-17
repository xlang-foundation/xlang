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

#include "function.h"

namespace X
{
	namespace Data 
	{
		class FunctionOp :
			public AST::Scope
		{
			std::vector<X::Value> m_funcs;
		public:
			FunctionOp()
			{
				m_Vars =
				{
					{"getcode",0},
				};
				//API: getcode
				{
					std::string name("getcode");
					auto f = [](X::XRuntime* rt, XObj* pThis,XObj* pContext,
						ARGS& params,
						KWARGS& kwParams,
						X::Value& retValue)
					{
						Function* pFuncObj = dynamic_cast<Function*>(pContext);
						auto code = pFuncObj->GetFunc()->getcode(false);
						retValue = X::Value(code);
						return true;
					};
					X::U_FUNC func(f);
					AST::ExternFunc* extFunc = new AST::ExternFunc(name,"retVal = getcode()",func);
					auto* pFuncObj = new Data::Function(extFunc, true);
					m_funcs.push_back(X::Value(pFuncObj));
				}
			}
			int QueryMethod(const char* name)
			{
				auto it = m_Vars.find(name);
				if (it != m_Vars.end())
				{
					return it->second;
				}
				else
				{
					return false;
				}
			}
			FORCE_INLINE virtual bool Get(XlangRuntime* rt, XObj* pContext,
				int idx, X::Value& v, X::LValue* lValue = nullptr)
			{
				v = m_funcs[idx];
				return true;
			}
			void clean()
			{
				m_funcs.clear();
			}
			virtual Scope* GetParentScope()
			{
				return nullptr;
			}
		};
		static FunctionOp* _function_op = nullptr;
		void Function::cleanup()
		{
			if (_function_op)
			{
				_function_op->clean();
				delete _function_op;
				_function_op = nullptr;
			}
		}
		void Function::GetBaseScopes(std::vector<AST::Scope*>& bases)
		{
			Object::GetBaseScopes(bases);
			if (_function_op == nullptr)
			{
				_function_op = new FunctionOp();
			}
			bases.push_back(_function_op);
		}
		int Function::QueryMethod(const char* name, int* pFlags)
		{
			return _function_op?_function_op->QueryMethod(name):-1;
		}
		bool Function::GetIndexValue(int idx, Value& v)
		{
			return _function_op?_function_op->Get(nullptr, nullptr, idx, v):false;
		}
		Function::Function(AST::Func* p, bool bOwnIt):Object()
		{
			m_ownFunc = bOwnIt;
			m_t = ObjType::Function;
			m_func = p;
		}
		Function::~Function()
		{
			if (m_ownFunc && m_func)
			{
				delete m_func;
			}
		}
		void Function::ChangeStatmentsIntoTranslateMode(bool changeIfStatment, 
			bool changeLoopStatment)
		{
			m_func->ChangeStatmentsIntoTranslateMode(changeIfStatment, changeLoopStatment);
		}
		bool Function::Call(XRuntime* rt, XObj* pContext,
			ARGS& params,
			KWARGS& kwParams,
			X::Value& retValue)
		{
			return m_func->Call(rt, pContext, params, kwParams, retValue);
		}
		bool Function::CallEx(XRuntime* rt, XObj* pContext, 
			ARGS& params, KWARGS& kwParams, X::Value& trailer, X::Value& retValue)
		{
			return m_func->CallEx(rt, pContext, params, kwParams, trailer,retValue);
		}
	}
}