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

#include "error_obj.h"
#include "scope.h"
#include "obj_func_scope.h"

namespace X
{
	namespace Data
	{
		static Obj_Func_Scope<2> _errorScope;
		void Error::Init()
		{
			_errorScope.Init();
			{
				auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					ARGS& params,
					KWARGS& kwParams,
					X::Value& retValue)
					{
						auto* pObj = dynamic_cast<Object*>(pContext);
						auto* pErrorObj = dynamic_cast<Error*>(pObj);
						retValue = X::Value(pErrorObj->GetCode());
						return true;
					};
				_errorScope.AddFunc("getCode", "obj.getCode()", f);
			}
			{
				auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					ARGS& params,
					KWARGS& kwParams,
					X::Value& retValue)
					{
						auto* pObj = dynamic_cast<Object*>(pContext);
						auto* pErrorObj = dynamic_cast<Error*>(pObj);
						std::string strInfo = pErrorObj->GetInfo();
						retValue = X::Value(strInfo);
						return true;
					};
				_errorScope.AddFunc("getInfo", "obj.getInfo()", f);
			}
		}
		void Error::cleanup()
		{
			_errorScope.Clean();
		}
		void Error::GetBaseScopes(std::vector<AST::Scope*>& bases)
		{
			bases.push_back(_errorScope.GetMyScope());
		}
	}
}