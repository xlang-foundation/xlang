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

#include "obj_func_scope.h"

namespace X
{
	namespace Data 
	{
		static Obj_Func_Scope<2> _funcScope;

		void Function::Init()
		{
			_funcScope.Init();
			//API: getcode
			{
				auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					ARGS& params,
					KWARGS& kwParams,
					X::Value& retValue)
					{
						Function* pFuncObj = dynamic_cast<Function*>(pContext);
						bool includeHeader = false;
						if (params.size() > 0 && params[0].IsBool())
						{
							includeHeader = (bool)params[0];
						}
						auto code = pFuncObj->GetFunc()->getcode(includeHeader);
						retValue = X::Value(code);
						return true;
					};
				_funcScope.AddFunc("getcode", "retVal = getcode(includeHeader=False)", f);
			}
			//API: call
			{
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
								bool bUseFirstParam = false;
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
										pThis, newParams, newKwParams, retValue); // Use new call signature maybe? No, calling inner func
									bUseFirstParam = true;
								} //End
								else if (firstParam.IsDict())
								{
									X::ARGS newParams;
									X::Dict dictArgs(firstParam);
									X::KWARGS newKwParams(1);
									dictArgs->Enum(
										[&newKwParams](X::Value& key, X::Value& val)
										{
											std::string strKey = key.asString();
											newKwParams.Add(strKey.c_str(), val,true);
										});
									bOK = pFuncObj->GetFunc()->Call(rt, pFuncObj,
										pThis, newParams, newKwParams, retValue);
									bUseFirstParam = true;
								}
								if (!bUseFirstParam)
								{
									//call with direct params
									//params passed in are arguments for the function
									bOK = pFuncObj->GetFunc()->Call(rt, pFuncObj,
										pThis, params, kwParams, retValue);
								}
							}
						}
						else
						{
							//call with no params
							bOK = pFuncObj->GetFunc()->Call(rt, pFuncObj,
								pThis, params, kwParams, retValue);
						}

						return bOK;
					};
				_funcScope.AddFunc("call", "retVal = func.call(dynamic parameters)", f);
			}
			_funcScope.Close();
		}
		void Function::cleanup()
		{
			_funcScope.Clean();
		}
		void Function::GetBaseScopes(std::vector<AST::Scope*>& bases)
		{
			Object::GetBaseScopes(bases);
			bases.push_back(_funcScope.GetMyScope());
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