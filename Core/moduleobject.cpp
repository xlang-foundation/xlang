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

#include "moduleobject.h"
#include "function.h"
#include "Hosting.h"
#include "obj_func_scope.h"

namespace X
{
	namespace AST
	{
		static Obj_Func_Scope<5> _listScope;
		void ModuleObject::Init()
		{
			_listScope.Init();
			//API: runfragment
			{
				auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					ARGS& params,
					KWARGS& kwParams,
					X::Value& retValue)
					{
						bool bPost = false;
						auto kwIt = kwParams.find("post");
						if (kwIt)
						{
							bPost = (bool)kwIt->val;
						}
						if (params.size() > 0)
						{
							std::string code;
							auto& v0 = params[0];
							std::string codePack;
							if (v0.IsObject()
								&& v0.GetObj()->GetType() == ObjType::Function)
							{
								auto* pFuncObj = dynamic_cast<X::Data::Function*>(v0.GetObj());
								code = pFuncObj->GetFunc()->GetCode();
							}
							else
							{
								code = v0.ToString();
							}
							auto* pModuleObj = dynamic_cast<ModuleObject*>(pContext);
							if (bPost)
							{
								Hosting::I().PostRunFragmentInMainThread(pModuleObj, code);
							}
							else
							{
								return Hosting::I().RunFragmentInModule(pModuleObj,
									code.c_str(), code.size(), retValue);
							}
						}
						else
						{
							retValue = X::Value(true);
							return true;
						}
					};
				_listScope.AddFunc("runfragment", "runfragment(code)", f);
			}
			//API: setprimitive
			{
				std::string name("setprimitive");
				auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					ARGS& params,
					KWARGS& kwParams,
					X::Value& retValue)
					{
						if (params.size() == 2)
						{
							std::string name = params[0].ToString();
							auto* pModuleObj = dynamic_cast<ModuleObject*>(pContext);
							pModuleObj->M()->SetPrimitive(name, params[1], rt);
						}
						return true;
					};
				_listScope.AddFunc("setprimitive", "setprimitive(key,func)", f);
			}
			//API: addcache
			{
				std::string name("addcache");
				auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					ARGS& params,
					KWARGS& kwParams,
					X::Value& retValue)
					{
						if (params.size() == 2)
						{
							std::string name = params[0].ToString();
							auto* pModuleObj = dynamic_cast<ModuleObject*>(pContext);
							pModuleObj->M()->AddModuleCache(name, params[1]);
						}
						return true;
					};
				_listScope.AddFunc("addcache", "addcache(key,value)", f);
			}
			//API: getcache
			{
				std::string name("getcache");
				auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					ARGS& params,
					KWARGS& kwParams,
					X::Value& retValue)
					{
						if (params.size() == 1)
						{
							std::string name = params[0].ToString();
							auto* pModuleObj = dynamic_cast<ModuleObject*>(pContext);
							retValue = pModuleObj->M()->GetModuleCache(name);
						}
						return true;
					};
				_listScope.AddFunc("getcache", "value = getcache(key)", f);
			}
			//API: removecache
			{
				std::string name("removecache");
				auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					ARGS& params,
					KWARGS& kwParams,
					X::Value& retValue)
					{
						if (params.size() == 1)
						{
							std::string name = params[0].ToString();
							auto* pModuleObj = dynamic_cast<ModuleObject*>(pContext);
							pModuleObj->M()->RemoveModuleCache(name);
						}
						return true;
					};
				_listScope.AddFunc("removecache", "removecache(key)", f);
			}
			_listScope.Close();
		}
		void ModuleObject::cleanup()
		{
			_listScope.Clean();
		}
		void ModuleObject::GetBaseScopes(std::vector<Scope*>& bases)
		{
			Object::GetBaseScopes(bases);
			bases.push_back(_listScope.GetMyScope());
			bases.push_back(m_pModule->GetMyScope());
		}
		int ModuleObject::QueryMethod(const char* name, int* pFlags)
		{
			std::string strName(name);
			int idx = _listScope.GetMyScope()->AddOrGet(strName,true);
			if (idx >= 0)
			{
				return -2-idx;//start from -2,then -3...
			}
			else
			{
				std::string strName(name);
				SCOPE_FAST_CALL_AddOrGet0(retIdx,m_pModule->GetMyScope(), strName, true);
				return retIdx;
			}
		}
		bool ModuleObject::GetIndexValue(int idx, Value& v)
		{
			if (idx <= -2)
			{
				_listScope.GetMyScope()->Get(nullptr, nullptr, -idx-2, v);
				return true;
			}
			else
			{
				m_pModule->GetStack()->Get(idx, v);
				return true;
			}
		}
		long long ModuleObject::Size()
		{
			if (m_pModule)
			{
				auto* pMyScope = m_pModule->GetMyScope();
				if (pMyScope)
				{
					return pMyScope->GetVarNum();
				}
			}
			return 0;
		}
	}
}