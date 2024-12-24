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

#include "bin.h"
#include "obj_func_scope.h"

namespace X
{
	namespace Data
	{
		static Obj_Func_Scope<1> _binScope;
		void Binary::Init()
		{
			_binScope.Init();
            {
                auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
                    ARGS& params,
                    KWARGS& kwParams,
                    X::Value& retValue)
                    {
                        // Get the binary object
                        auto* pObj = dynamic_cast<Binary*>(pContext);
                        if (!pObj)
                        {
                            retValue = X::Value();
                            return false;
                        }

                        // Retrieve the encoding type from the parameters
                        std::string encodeType;
                        if (params.size() > 0)
                        {
                            encodeType = params[0].ToString();
                        }

                        std::string strRet;

                        // Convert binary data to string based on the encoding type
                        if (encodeType.empty() || encodeType == "utf8")
                        {
                            // Interpret the binary data as UTF-8 string
                            strRet = std::string(pObj->Data(), pObj->Size());
                        }
                        else if (encodeType == "utf16")
                        {
#if (WIN32)
                            // On Windows, use the Windows API to convert UTF-16 to UTF-8
                            auto utf16Str = reinterpret_cast<const wchar_t*>(pObj->Data());
                            int size = WideCharToMultiByte(CP_UTF8, 0, utf16Str, 
                                pObj->Size() / 2, nullptr, 0, nullptr, nullptr);
                            if (size > 0)
                            {
                                std::vector<char> utf8Buffer(size);
                                WideCharToMultiByte(CP_UTF8, 0, utf16Str, 
                                    pObj->Size()/ 2, utf8Buffer.data(), size, nullptr, nullptr);
                                strRet = std::string(utf8Buffer.begin(), utf8Buffer.end());
                            }
#else
                            // For non-Windows platforms, conversion may require iconv or equivalent
                            rt->Print("UTF-16 encoding is not supported on this platform.");
                            retValue = X::Value();
                            return false;
#endif
                        }
                        else if (encodeType == "ascii")
                        {
                            // Interpret the binary data as ASCII
                            strRet = std::string(pObj->Data(), pObj->Size());
                        }
                        else
                        {
                            // Unsupported encoding type
                            retValue = X::Value();
                            return true;
                        }

                        // Set the return value
                        retValue = X::Value(strRet);
                        return true;
                    };

                _binScope.AddFunc("convert_to_str", "s = convert_to_str(encoding_type)", f);
            }

		}
		void Binary::cleanup()
		{
			_binScope.Clean();
		}
        void Binary::GetBaseScopes(std::vector<AST::Scope*>& bases)
        {
            bases.push_back(_binScope.GetMyScope());
        }
	}
}