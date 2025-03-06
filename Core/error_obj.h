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
#include <string>
namespace X
{
	namespace Data
	{
		class Error :
			virtual public XError,
			virtual public Object
		{
		protected:
			std::string m_s;
			int m_code;
		public:
			static void Init();
			static void cleanup();
			Error(int code, std::string& str)
			{
				m_t = ObjType::Error;
				m_s = str;
				m_code = code;
			}
			FORCE_INLINE virtual const char* GetInfo() override
			{
				return m_s.c_str();
			}
			FORCE_INLINE virtual int GetCode() override
			{
				return m_code;
			}
			virtual void GetBaseScopes(std::vector<AST::Scope*>& bases) override;
		};
	} // namespace Data
} // namespace X