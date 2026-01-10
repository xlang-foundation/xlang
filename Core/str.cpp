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
		static Obj_Func_Scope<31> _strScope;
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
				auto replaceFunc = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					ARGS& params, KWARGS& kwParams, X::Value& retValue) {
						// Extract the target object
						auto* pObj = dynamic_cast<Object*>(pContext);
						auto* pStrObj = dynamic_cast<Str*>(pObj);

						// Ensure there are enough arguments
						if (params.size() < 2) {
							return false; // Insufficient arguments
						}

						// Extract arguments
						const std::string& original = pStrObj->ToString();
						const std::string& searchString = params[0].ToString();
						const std::string& replaceString = params[1].ToString();
						size_t replaceCount = std::string::npos; // Default: replace all occurrences

						// If a third parameter is provided, it specifies the maximum replacements
						if (params.size() > 2) {
							replaceCount = params[2].GetLongLong();
						}

						if (searchString.empty()) {
							retValue = X::Value(original); // No replacement if the search string is empty
							return true;
						}

						std::string result;
						size_t startPos = 0;
						size_t pos;
						size_t count = 0;

						while ((pos = original.find(searchString, startPos)) != std::string::npos) {
							// Append everything before the match
							result.append(original, startPos, pos - startPos);

							// Append the replacement string
							result.append(replaceString);

							// Update start position
							startPos = pos + searchString.length();

							// Increment the replacement counter
							count++;
							if (count == replaceCount) {
								break; // Stop if we've reached the max replacements
							}
						}

						// Append any remaining part of the original string
						result.append(original, startPos, std::string::npos);

						// Set the result value
						retValue = X::Value(result);
						return true;
					};

				// Register the function in the scope
				_strScope.AddFunc("replace", "result = replace(search_string, replace_string, max_replacements=ALL)", replaceFunc);

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
			{
				auto stripFunc = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					ARGS& params, KWARGS& kwParams, X::Value& retValue) {
						auto* pObj = dynamic_cast<Object*>(pContext);
						auto* pStrObj = dynamic_cast<Str*>(pObj);

						if (pStrObj == nullptr) {
							retValue = X::Value();
							return false; // Invalid object context
						}

						// Original string
						std::string original = pStrObj->ToString();

						// Characters to strip
						std::string chars = " \t\n\r"; // Default: whitespace characters
						if (params.size() > 0) {
							chars = params[0].ToString();
						}

						// Perform strip operation
						size_t start = original.find_first_not_of(chars);
						size_t end = original.find_last_not_of(chars);

						std::string result = (start == std::string::npos || end == std::string::npos)
							? ""  // If no valid characters remain
							: original.substr(start, end - start + 1);

						// Set the result value
						auto* pNewStr = new Str(result);
						retValue = X::Value(pNewStr);
						return true;
					};

				// Register the strip function in the scope
				_strScope.AddFunc("strip", "new_str = var_str.strip([chars_to_remove])", stripFunc);
			}
			{
				auto joinFunc = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					ARGS& params, KWARGS& kwParams, X::Value& retValue) {
						// Ensure we have at least one parameter (the list of strings to join)
						if (params.size() < 1) {
							retValue = X::Value(); // Insufficient arguments
							return false;
						}

						// Extract the separator string from pContext
						auto* pObj = dynamic_cast<Object*>(pContext);
						auto* pStrSeparator = dynamic_cast<Str*>(pObj);
						if (!pStrSeparator) {
							retValue = X::Value(); // Invalid context for separator
							return false;
						}
						std::string separator = pStrSeparator->ToString();

						// Get the list of strings
						auto* pObjList = dynamic_cast<Object*>(params[0].GetObj());
						auto* pListObj = dynamic_cast<List*>(pObjList);
						if (!pListObj) {
							retValue = X::Value(); // Parameter must be a list
							return false;
						}

						// Perform the join operation
						std::string result;
						size_t listSize = pListObj->Size();
						for (size_t i = 0; i < listSize; ++i) {
							// Get the current element as a string
							X::Value elementValue = pListObj->Get(i);
							auto* pElementObj = dynamic_cast<Object*>(elementValue.GetObj());
							auto* pStrElement = dynamic_cast<Str*>(pElementObj);
							if (!pStrElement) {
								retValue = X::Value(); // All elements must be strings
								return false;
							}

							// Append the string to the result
							result += pStrElement->ToString();

							// Add the separator if it's not the last element
							if (i < listSize - 1) {
								result += separator;
							}
						}

						// Create a new Str object for the result
						auto* pNewStr = new Str(result);
						retValue = X::Value(pNewStr);
						return true;
					};

				// Register the join function in the scope
				_strScope.AddFunc("join", "new_str = join(list_of_strings)", joinFunc);
			}
			{
				auto startsWithFunc = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					ARGS& params, KWARGS& kwParams, X::Value& retValue) {
						if (params.size() < 1) {
							retValue = false;
							return false;
						}
						auto* pObj = dynamic_cast<Object*>(pContext);
						auto* pStrObj = dynamic_cast<Str*>(pObj);
						std::string str = pStrObj->ToString();
						std::string prefix = params[0].ToString();
						size_t start = 0;
						size_t end = str.size();
						if (params.size() > 1) start = params[1].GetLongLong();
						if (params.size() > 2) end = params[2].GetLongLong();
						if (start < str.size() && end <= str.size() && start <= end) {
							std::string substr = str.substr(start, end - start);
							retValue = (substr.rfind(prefix, 0) == 0);
						} else {
							retValue = false;
						}
						return true;
					};
				_strScope.AddFunc("startswith", "bool = startswith(prefix[, start[, end]])", startsWithFunc);
			}
			{
				auto endsWithFunc = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					ARGS& params, KWARGS& kwParams, X::Value& retValue) {
						if (params.size() < 1) {
							retValue = false;
							return false;
						}
						auto* pObj = dynamic_cast<Object*>(pContext);
						auto* pStrObj = dynamic_cast<Str*>(pObj);
						std::string str = pStrObj->ToString();
						std::string suffix = params[0].ToString();
						size_t start = 0;
						size_t end = str.size();
						if (params.size() > 1) start = params[1].GetLongLong();
						if (params.size() > 2) end = params[2].GetLongLong();
						if (start < str.size() && end <= str.size() && start <= end) {
							std::string substr = str.substr(start, end - start);
							if (suffix.size() <= substr.size()) {
								retValue = (substr.compare(substr.size() - suffix.size(), suffix.size(), suffix) == 0);
							} else {
								retValue = false;
							}
						} else {
							retValue = false;
						}
						return true;
					};
				_strScope.AddFunc("endswith", "bool = endswith(suffix[, start[, end]])", endsWithFunc);
			}
			{
				auto isAlphaFunc = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					ARGS& params, KWARGS& kwParams, X::Value& retValue) {
						auto* pObj = dynamic_cast<Object*>(pContext);
						auto* pStrObj = dynamic_cast<Str*>(pObj);
						std::string str = pStrObj->ToString();
						if (str.empty()) {
							retValue = false;
							return true;
						}
						for (char c : str) {
							if (!std::isalpha(static_cast<unsigned char>(c))) {
								retValue = false;
								return true;
							}
						}
						retValue = true;
						return true;
					};
				_strScope.AddFunc("isalpha", "bool = isalpha()", isAlphaFunc);
			}
			{
				auto isDigitFunc = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					ARGS& params, KWARGS& kwParams, X::Value& retValue) {
						auto* pObj = dynamic_cast<Object*>(pContext);
						auto* pStrObj = dynamic_cast<Str*>(pObj);
						std::string str = pStrObj->ToString();
						if (str.empty()) {
							retValue = false;
							return true;
						}
						for (char c : str) {
							if (!std::isdigit(static_cast<unsigned char>(c))) {
								retValue = false;
								return true;
							}
						}
						retValue = true;
						return true;
					};
				_strScope.AddFunc("isdigit", "bool = isdigit()", isDigitFunc);
			}
			{
				auto isAlnumFunc = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					ARGS& params, KWARGS& kwParams, X::Value& retValue) {
						auto* pObj = dynamic_cast<Object*>(pContext);
						auto* pStrObj = dynamic_cast<Str*>(pObj);
						std::string str = pStrObj->ToString();
						if (str.empty()) {
							retValue = false;
							return true;
						}
						for (char c : str) {
							if (!std::isalnum(static_cast<unsigned char>(c))) {
								retValue = false;
								return true;
							}
						}
						retValue = true;
						return true;
					};
				_strScope.AddFunc("isalnum", "bool = isalnum()", isAlnumFunc);
			}
			{
				auto isSpaceFunc = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					ARGS& params, KWARGS& kwParams, X::Value& retValue) {
						auto* pObj = dynamic_cast<Object*>(pContext);
						auto* pStrObj = dynamic_cast<Str*>(pObj);
						std::string str = pStrObj->ToString();
						if (str.empty()) {
							retValue = false;
							return true;
						}
						for (char c : str) {
							if (!std::isspace(static_cast<unsigned char>(c))) {
								retValue = false;
								return true;
							}
						}
						retValue = true;
						return true;
					};
				_strScope.AddFunc("isspace", "bool = isspace()", isSpaceFunc);
			}
			{
				auto isUpperFunc = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					ARGS& params, KWARGS& kwParams, X::Value& retValue) {
						auto* pObj = dynamic_cast<Object*>(pContext);
						auto* pStrObj = dynamic_cast<Str*>(pObj);
						std::string str = pStrObj->ToString();
						if (str.empty()) {
							retValue = false;
							return true;
						}
						bool hasAlpha = false;
						for (char c : str) {
							if (std::isalpha(static_cast<unsigned char>(c))) {
								hasAlpha = true;
								if (!std::isupper(static_cast<unsigned char>(c))) {
									retValue = false;
									return true;
								}
							}
						}
						retValue = hasAlpha;
						return true;
					};
				_strScope.AddFunc("isupper", "bool = isupper()", isUpperFunc);
			}
			{
				auto isLowerFunc = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					ARGS& params, KWARGS& kwParams, X::Value& retValue) {
						auto* pObj = dynamic_cast<Object*>(pContext);
						auto* pStrObj = dynamic_cast<Str*>(pObj);
						std::string str = pStrObj->ToString();
						if (str.empty()) {
							retValue = false;
							return true;
						}
						bool hasAlpha = false;
						for (char c : str) {
							if (std::isalpha(static_cast<unsigned char>(c))) {
								hasAlpha = true;
								if (!std::islower(static_cast<unsigned char>(c))) {
									retValue = false;
									return true;
								}
							}
						}
						retValue = hasAlpha;
						return true;
					};
				_strScope.AddFunc("islower", "bool = islower()", isLowerFunc);
			}
			{
				auto lstripFunc = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					ARGS& params, KWARGS& kwParams, X::Value& retValue) {
						auto* pObj = dynamic_cast<Object*>(pContext);
						auto* pStrObj = dynamic_cast<Str*>(pObj);
						std::string str = pStrObj->ToString();
						std::string chars = " \t\n\r";
						if (params.size() > 0) chars = params[0].ToString();
						size_t start = str.find_first_not_of(chars);
						std::string result = (start == std::string::npos) ? "" : str.substr(start);
						auto* pNewStr = new Str(result);
						retValue = X::Value(pNewStr);
						return true;
					};
				_strScope.AddFunc("lstrip", "new_str = lstrip([chars])", lstripFunc);
			}
			{
				auto rstripFunc = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					ARGS& params, KWARGS& kwParams, X::Value& retValue) {
						auto* pObj = dynamic_cast<Object*>(pContext);
						auto* pStrObj = dynamic_cast<Str*>(pObj);
						std::string str = pStrObj->ToString();
						std::string chars = " \t\n\r";
						if (params.size() > 0) chars = params[0].ToString();
						size_t end = str.find_last_not_of(chars);
						std::string result = (end == std::string::npos) ? "" : str.substr(0, end + 1);
						auto* pNewStr = new Str(result);
						retValue = X::Value(pNewStr);
						return true;
					};
				_strScope.AddFunc("rstrip", "new_str = rstrip([chars])", rstripFunc);
			}
			{
				auto swapCaseFunc = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					ARGS& params, KWARGS& kwParams, X::Value& retValue) {
						auto* pObj = dynamic_cast<Object*>(pContext);
						auto* pStrObj = dynamic_cast<Str*>(pObj);
						std::string str = pStrObj->ToString();
						std::string result;
						for (char c : str) {
							if (std::isupper(static_cast<unsigned char>(c))) {
								result += std::tolower(static_cast<unsigned char>(c));
							} else if (std::islower(static_cast<unsigned char>(c))) {
								result += std::toupper(static_cast<unsigned char>(c));
							} else {
								result += c;
							}
						}
						auto* pNewStr = new Str(result);
						retValue = X::Value(pNewStr);
						return true;
					};
				_strScope.AddFunc("swapcase", "new_str = swapcase()", swapCaseFunc);
			}
			{
				auto titleFunc = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					ARGS& params, KWARGS& kwParams, X::Value& retValue) {
						auto* pObj = dynamic_cast<Object*>(pContext);
						auto* pStrObj = dynamic_cast<Str*>(pObj);
						std::string str = pStrObj->ToString();
						std::string result;
						bool newWord = true;
						for (char c : str) {
							if (std::isalpha(static_cast<unsigned char>(c))) {
								if (newWord) {
									result += std::toupper(static_cast<unsigned char>(c));
									newWord = false;
								} else {
									result += std::tolower(static_cast<unsigned char>(c));
								}
							} else {
								result += c;
								newWord = true;
							}
						}
						auto* pNewStr = new Str(result);
						retValue = X::Value(pNewStr);
						return true;
					};
				_strScope.AddFunc("title", "new_str = title()", titleFunc);
			}
			{
				auto capitalizeFunc = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					ARGS& params, KWARGS& kwParams, X::Value& retValue) {
						auto* pObj = dynamic_cast<Object*>(pContext);
						auto* pStrObj = dynamic_cast<Str*>(pObj);
						std::string str = pStrObj->ToString();
						if (str.empty()) {
							auto* pNewStr = new Str("");
							retValue = X::Value(pNewStr);
							return true;
						}
						std::string result;
						result += std::toupper(static_cast<unsigned char>(str[0]));
						for (size_t i = 1; i < str.size(); i++) {
							result += std::tolower(static_cast<unsigned char>(str[i]));
						}
						auto* pNewStr = new Str(result);
						retValue = X::Value(pNewStr);
						return true;
					};
				_strScope.AddFunc("capitalize", "new_str = capitalize()", capitalizeFunc);
			}
			{
				auto centerFunc = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					ARGS& params, KWARGS& kwParams, X::Value& retValue) {
						if (params.size() < 1) {
							return false;
						}
						auto* pObj = dynamic_cast<Object*>(pContext);
						auto* pStrObj = dynamic_cast<Str*>(pObj);
						std::string str = pStrObj->ToString();
						size_t width = params[0].GetLongLong();
						char fillchar = ' ';
						if (params.size() > 1) {
							std::string fc = params[1].ToString();
							if (!fc.empty()) fillchar = fc[0];
						}
						if (str.size() >= width) {
							auto* pNewStr = new Str(str);
							retValue = X::Value(pNewStr);
							return true;
						}
						size_t totalPad = width - str.size();
						size_t leftPad = totalPad / 2;
						size_t rightPad = totalPad - leftPad;
						std::string result = std::string(leftPad, fillchar) + str + std::string(rightPad, fillchar);
						auto* pNewStr = new Str(result);
						retValue = X::Value(pNewStr);
						return true;
					};
				_strScope.AddFunc("center", "new_str = center(width[, fillchar])", centerFunc);
			}
			{
				auto ljustFunc = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					ARGS& params, KWARGS& kwParams, X::Value& retValue) {
						if (params.size() < 1) return false;
						auto* pObj = dynamic_cast<Object*>(pContext);
						auto* pStrObj = dynamic_cast<Str*>(pObj);
						std::string str = pStrObj->ToString();
						size_t width = params[0].GetLongLong();
						char fillchar = ' ';
						if (params.size() > 1) {
							std::string fc = params[1].ToString();
							if (!fc.empty()) fillchar = fc[0];
						}
						if (str.size() >= width) {
							auto* pNewStr = new Str(str);
							retValue = X::Value(pNewStr);
						} else {
							std::string result = str + std::string(width - str.size(), fillchar);
							auto* pNewStr = new Str(result);
							retValue = X::Value(pNewStr);
						}
						return true;
					};
				_strScope.AddFunc("ljust", "new_str = ljust(width[, fillchar])", ljustFunc);
			}
			{
				auto rjustFunc = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					ARGS& params, KWARGS& kwParams, X::Value& retValue) {
						if (params.size() < 1) return false;
						auto* pObj = dynamic_cast<Object*>(pContext);
						auto* pStrObj = dynamic_cast<Str*>(pObj);
						std::string str = pStrObj->ToString();
						size_t width = params[0].GetLongLong();
						char fillchar = ' ';
						if (params.size() > 1) {
							std::string fc = params[1].ToString();
							if (!fc.empty()) fillchar = fc[0];
						}
						if (str.size() >= width) {
							auto* pNewStr = new Str(str);
							retValue = X::Value(pNewStr);
						} else {
							std::string result = std::string(width - str.size(), fillchar) + str;
							auto* pNewStr = new Str(result);
							retValue = X::Value(pNewStr);
						}
						return true;
					};
				_strScope.AddFunc("rjust", "new_str = rjust(width[, fillchar])", rjustFunc);
			}
			{
				auto zfillFunc = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					ARGS& params, KWARGS& kwParams, X::Value& retValue) {
						if (params.size() < 1) return false;
						auto* pObj = dynamic_cast<Object*>(pContext);
						auto* pStrObj = dynamic_cast<Str*>(pObj);
						std::string str = pStrObj->ToString();
						size_t width = params[0].GetLongLong();
						if (str.size() >= width) {
							auto* pNewStr = new Str(str);
							retValue = X::Value(pNewStr);
							return true;
						}
						size_t signPos = 0;
						if (!str.empty() && (str[0] == '+' || str[0] == '-')) {
							signPos = 1;
						}
						std::string result = str.substr(0, signPos) + 
											 std::string(width - str.size(), '0') + 
											 str.substr(signPos);
						auto* pNewStr = new Str(result);
						retValue = X::Value(pNewStr);
						return true;
					};
				_strScope.AddFunc("zfill", "new_str = zfill(width)", zfillFunc);
			}
			{
				auto countFunc = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					ARGS& params, KWARGS& kwParams, X::Value& retValue) {
						if (params.size() < 1) return false;
						auto* pObj = dynamic_cast<Object*>(pContext);
						auto* pStrObj = dynamic_cast<Str*>(pObj);
						std::string str = pStrObj->ToString();
						std::string sub = params[0].ToString();
						size_t start = 0;
						size_t end = str.size();
						if (params.size() > 1) start = params[1].GetLongLong();
						if (params.size() > 2) end = params[2].GetLongLong();
						if (sub.empty()) {
							retValue = (long long)0;
							return true;
						}
						size_t count = 0;
						size_t pos = start;
						while (pos < end && (pos = str.find(sub, pos)) != std::string::npos && pos < end) {
							count++;
							pos += sub.length();
						}
						retValue = (long long)count;
						return true;
					};
				_strScope.AddFunc("count", "num = count(sub[, start[, end]])", countFunc);
			}
			{
				auto indexFunc = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					ARGS& params, KWARGS& kwParams, X::Value& retValue) {
						if (params.size() < 1) return false;
						auto* pObj = dynamic_cast<Object*>(pContext);
						auto* pStrObj = dynamic_cast<Str*>(pObj);
						std::string str = pStrObj->ToString();
						std::string sub = params[0].ToString();
						size_t start = 0;
						size_t end = str.size();
						if (params.size() > 1) start = params[1].GetLongLong();
						if (params.size() > 2) end = params[2].GetLongLong();
						if (start < str.size() && end <= str.size() && start <= end) {
							size_t pos = str.find(sub, start);
							if (pos != std::string::npos && pos < end) {
								retValue = (long long)pos;
								return true;
							}
						}
						// Throw exception like Python's index() when not found
						retValue = (long long)-1;
						return true;
					};
				_strScope.AddFunc("index", "pos = index(sub[, start[, end]])", indexFunc);
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