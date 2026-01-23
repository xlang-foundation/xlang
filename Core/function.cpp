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
				//API: call
				{
					std::string name("call");
					auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
						ARGS& params,
						KWARGS& kwParams,
						X::Value& retValue)
						{
							bool bOK = false;
							Function* pFuncObj = dynamic_cast<Function*>(pContext);
							if (params.size() > 0)
							{
								//if one parameter and it is list used as params
								//or dict, we use it as kwParams
								//also support first one is list, second is dict
								if (params.size() >= 1)
								{
									X::Value& firstParam = params[0];
									if (firstParam.IsList())
									{
										X::List listArgs(firstParam);
										X::ARGS newParams(listArgs->Size());
										for (auto li : *listArgs)
										{
											newParams.push_back(li);
										}
										X::KWARGS newKwParams;
										if (params.size() >= 2)
										{
											//second param as kwParams
											X::Value& secondParam = params[1];
											if (secondParam.IsDict())
											{
												X::Dict dictArgs(secondParam);
												dictArgs->Enum(
													[&newKwParams](X::Value& key, X::Value& val)
													{
														std::string strKey = key.asString();
														newKwParams.Add(strKey.c_str(), val);
													});
											}
										}
										bOK = pFuncObj->GetFunc()->Call(rt, pFuncObj,
											pThis, newParams, newKwParams, retValue);
									} //End
									else if(firstParam.IsDict())
									{
										X::ARGS newParams;
										X::Dict dictArgs(firstParam);
										X::KWARGS newKwParams;
										dictArgs->Enum(
											[&newKwParams](X::Value& key, X::Value& val)
											{
												std::string strKey = key.asString();
												newKwParams.Add(strKey.c_str(), val);
											});
										bOK = pFuncObj->GetFunc()->Call(rt, pFuncObj,
											pThis, newParams, newKwParams, retValue);
									}
								}
							}

							return bOK;
						};
					X::U_FUNC func(f);
					AST::ExternFunc* extFunc = new AST::ExternFunc(name, "retVal = func.call(dynamic parameters)", func);
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
		bool Function::GetIndexValue(long long idx, Value& v)
		{
			return _function_op?_function_op->Get(nullptr, nullptr, (int)idx, v):false;
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
			return m_func->Call(rt, this,pContext, params, kwParams, retValue);
		}
		bool Function::IsFuncEx()
		{
			if(m_func->IsBuiltinFunc())
			{
				X::AST::ExternFunc* pExtFunc = dynamic_cast<X::AST::ExternFunc*>(m_func);
				if (pExtFunc)
				{
					return pExtFunc->IsFuncEx();
				}
			}
			return false;
		}
		bool Function::CallEx(XRuntime* rt, XObj* pContext, 
			ARGS& params, KWARGS& kwParams, X::Value& trailer, X::Value& retValue)
		{
			return m_func->CallEx(rt, this,pContext, params, kwParams, trailer,retValue);
		}
	}
}