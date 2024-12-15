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

#include "str.h"
#include "scope.h"
#include "list.h"
#include "function.h"
#include <string>
#include <regex>
#include <sstream>
#include <iterator>
#include "constexpr.h"
#include "obj_func_scope.h"

namespace X
{
	namespace Data
	{
		static Obj_Func_Scope<10> _strScope;
		void Str::Init()
		{
			_strScope.Init();
			{
				auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					ARGS& params,
					KWARGS& kwParams,
					X::Value& retValue)
				{
					auto* pObj = dynamic_cast<Object*>(pContext);
					auto* pStrObj = dynamic_cast<Str*>(pObj);
					std::string x = params[0].ToString();
					size_t offset = 0;
					if (params.size() > 1)
					{
						offset = params[1].GetLongLong();
					}
					auto pos = pStrObj->Find(x, offset);
					retValue = X::Value((long long)pos);
					return true;
				};
				_strScope.AddFunc("find", "pos = find(search_string)", f);
			}
			{
				auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					ARGS& params,
					KWARGS& kwParams,
					X::Value& retValue)
				{
					auto* pObj = dynamic_cast<Object*>(pContext);
					auto* pStrObj = dynamic_cast<Str*>(pObj);
					std::string x = params[0].ToString();
					size_t offset = std::string::npos;
					if (params.size() > 1)
					{
						offset = params[1].GetLongLong();
					}
					auto pos = pStrObj->RFind(x, offset);
					retValue = X::Value((long long)pos);
					return true;
				};
				_strScope.AddFunc("rfind", "pos = rfind(search_string)", f);
			}
			{
				auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					ARGS& params,
					KWARGS& kwParams,
					X::Value& retValue)
				{
					auto* pObj = dynamic_cast<Object*>(pContext);
					auto* pStrObj = dynamic_cast<Str*>(pObj);
					size_t start = 0;
					size_t end = -1;
					if (params.size() >= 1)
					{
						start = params[0].GetLongLong();
					}
					if (params.size() >= 2)
					{
						end = params[1].GetLongLong();
					}
					std::string retStr;
					pStrObj->Slice(start, end, retStr);
					Str* pNewStr = new Str((const char*)retStr.c_str(), (int)retStr.size());
					retValue = X::Value(pNewStr);
					return true;
				};
				_strScope.AddFunc("slice", "newStr = var_str.slice(startPos[,endPos])", f);
			}
			{
				auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					ARGS& params,
					KWARGS& kwParams,
					X::Value& retValue)
				{
					auto* pObj = dynamic_cast<Object*>(pContext);
					auto* pStrObj = dynamic_cast<Str*>(pObj);
					size_t  size = 0;
					if (pStrObj)
					{
						size = pStrObj->GetSize();
					}
					retValue = X::Value((long long)size);
					return true;
				};
				_strScope.AddFunc("size", "size()", f);
			}
			{
				auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					ARGS& params,
					KWARGS& kwParams,
					X::Value& retValue)
				{
					std::string delim("\n");
					if (params.size() >= 1)
					{
						delim = params[0].ToString();
					}
					auto* pObj = dynamic_cast<Object*>(pContext);
					auto* pStrObj = dynamic_cast<Str*>(pObj);
					if (pStrObj == nullptr)
					{
						retValue = X::Value();
						return false;
					}
					std::vector<std::string> li;
					pStrObj->Split(delim, li);
					auto* pList = new List(li);
					pList->IncRef();
					XObj* pObjList = dynamic_cast<XObj*>(pList);
					retValue = X::Value(pObjList, false);
					return true;
				};
				_strScope.AddFunc("split", "item_list = var_str.split(delimiter_str)", f);
			}
			{
				auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					ARGS& params,
					KWARGS& kwParams,
					X::Value& retValue)
				{
					std::string delim("\n");
					if (params.size() >= 1)
					{
						delim = params[0].ToString();
					}
					auto* pObj = dynamic_cast<Object*>(pContext);
					auto* pStrObj = dynamic_cast<Str*>(pObj);
					std::vector<std::string> li;
					pStrObj->SplitWithChars(delim, li);
					auto* pList = new List(li);
					pList->IncRef();
					XObj* pObjList = dynamic_cast<XObj*>(pList);
					retValue = X::Value(pObjList, false);
					return true;
				};
				_strScope.AddFunc("splitWithChars", "item_list = var_str.splitWithChars(delimiter_chars)", f);
			}
			{
				auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					ARGS& params,
					KWARGS& kwParams,
					X::Value& retValue)
				{
					auto* pObj = dynamic_cast<Object*>(pContext);
					auto* pStrObj = dynamic_cast<Str*>(pObj);
					auto str_abi = pStrObj->ToString();
					std::string strVal(str_abi);
					g_pXHost->ReleaseString(str_abi);
					std::transform(strVal.begin(),
						strVal.end(), strVal.begin(),
						[](unsigned char c) { return std::toupper(c); });
					auto* pNewStr = new Str(strVal);
					retValue = X::Value(pNewStr);
					return true;
				};
				_strScope.AddFunc("toupper", "new_str = toupper()", f);
			}
			{
				auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					ARGS& params,
					KWARGS& kwParams,
					X::Value& retValue)
				{
					auto* pObj = dynamic_cast<Object*>(pContext);
					auto* pStrObj = dynamic_cast<Str*>(pObj);
					std::string strVal = pStrObj->ToString();
					std::transform(strVal.begin(),
						strVal.end(), strVal.begin(),
						[](unsigned char c) { return std::tolower(c); });
					auto* pNewStr = new Str(strVal);
					retValue = X::Value(pNewStr);
					return true;
				};
				_strScope.AddFunc("tolower", "new_str = tolower()", f);
			}
			{
				auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					ARGS& params,
					KWARGS& kwParams,
					X::Value& retValue)
				{
					auto* pObj = dynamic_cast<Object*>(pContext);
					auto* pStrObj = dynamic_cast<Str*>(pObj);
					std::string pattern = params[0].ToString();
					const std::regex r(pattern);
					std::string target = params[1].ToString();
					auto str_abi = pStrObj->ToString();
					std::string org_str = str_abi;
					g_pXHost->ReleaseString(str_abi);
					std::stringstream result;
					std::regex_replace(std::ostream_iterator<char>(result),
						org_str.begin(), org_str.end(), r, target);
					auto* pNewStr = new Str(result.str());
					retValue = X::Value(pNewStr);
					return true;
				};
				_strScope.AddFunc("regex_replace", "new_str = regex_replace(regex_expr,target_chars)", f);
			}
			{
				auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					ARGS& params, KWARGS& kwParams, X::Value& retValue) 
					{
						// Ensure there are two parameters: the string to match and the regex pattern
						if (params.size() < 1) {
							retValue = false; // Indicate failure
							return true;
						}

						auto* pObj = dynamic_cast<Object*>(pContext);
						auto* pStrObj = dynamic_cast<Str*>(pObj);
						std::string target = pStrObj->ToString();
						std::string pattern = params[0].ToString();

						try {
							// Create a regex object and check for a match
							const std::regex r(pattern);
							bool match = std::regex_match(target, r);

							// Return the match result
							retValue = match;
							return true;
						}
						catch (const std::regex_error) {
							// Handle regex syntax errors
							retValue = false; // Indicate failure
							return true;
						}
					};
				// Add the `regex_match` function to the string scope
				_strScope.AddFunc("regex_match", "bool = string_var.regex_match(regex_expr)", f);
			}
			_strScope.Close();
		}
		void Str::cleanup()
		{
			_strScope.Clean();
		}
		void Str::GetBaseScopes(std::vector<AST::Scope*>& bases)
		{
			bases.push_back(_strScope.GetMyScope());
		}
		bool Str::Iterate(X::XRuntime* rt, XObj* pContext,
			IterateProc proc, ARGS& params, KWARGS& kwParams,
			X::Value& retValue)
		{
			ConstExpr* pFilter = dynamic_cast<ConstExpr*>(params[0].GetObj());
			ConstExpr* pAction = dynamic_cast<ConstExpr*>(params[1].GetObj());
			size_t size = m_s.size();
			for (size_t i = 0; i < size; i++)
			{
				bool bEnable = false;
				pFilter->Run(this, i, bEnable);
				if (bEnable)
				{
					pAction->Run(this, i, bEnable);
				}
			}
			return true;
		}
	}
}