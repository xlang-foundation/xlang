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

#include "jitlib.h"
#include "compiler.h"
#include "cppcompiler.h"

namespace X
{
	namespace Jit
	{
		JitLib::~JitLib()
		{
			for (auto* pCompiler : m_compilers)
			{
				if (pCompiler != nullptr)
				{
					delete pCompiler;
				}
			}
		}
		std::string JitLib::QuotePath(std::string& strSrc)
		{
			std::string strNew = strSrc;
			ReplaceAll(strNew, "\\", "\\\\");
			strNew = "\"" + strNew + "\"";
			return strNew;
		}
		bool JitLib::Build()
		{
			for (auto& funcInfo : m_funcs)
			{
				switch (funcInfo.langType)
				{
				case LangType::cpp:
					if (m_compilers[(int)LangType::cpp] == nullptr)
					{
						auto* pCompiler = new CppCompiler();
						pCompiler->SetLib(this);
						std::string useDefault;
						pCompiler->Init(useDefault);
						m_compilers[(int)LangType::cpp] = pCompiler;
					}
					break;
				default:
					break;
				}
			}
			for (int i = 0; i < (int)LangType::Count; i++)
			{
				JitCompiler* pCompiler = m_compilers[i];
				if (pCompiler == nullptr)
				{
					continue;
				}
				std::vector<std::string> srcs;
				std::vector<std::string> exports;
				BuildCodeAction action;
				pCompiler->BuildCode(srcs,exports,action);
				if (action == BuildCodeAction::NeedBuild)
				{
					pCompiler->CompileAndLink(srcs);
					pCompiler->LoadLib();
				}
			}
			return true;
		}
	}
}

