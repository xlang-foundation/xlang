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

#include <map>
#include <vector>
#include <string>

namespace X
{
	namespace Jit
	{
		class JitLib;
		enum class BuildCodeAction
		{
			NeedBuild,
			HashAllMached,
		};
		class JitCompiler
		{
		public:
			JitCompiler()
			{

			}
			~JitCompiler()
			{

			}
			void SetLib(JitLib* p)
			{
				mJitLib = p;
			}
			virtual bool Init(std::string& libFileName) = 0;
			virtual bool BuildCode(std::vector<std::string>& srcs, 
				std::vector<std::string>& exports, BuildCodeAction& action) = 0;
			virtual bool CompileAndLink(std::vector<std::string> srcs) = 0;
			virtual bool LoadLib() = 0;
			virtual void UnloadLib() = 0;
			void AddIncludeFile(std::string strFile)
			{
				mIncludes.push_back(strFile);
			}
		protected:
			JitLib* mJitLib = nullptr;
			std::string mJitFolder;
			std::string mLibFileName;
			void* mLibHandle = nullptr;
			std::vector<std::string> mIncludes;
		};
	}
}