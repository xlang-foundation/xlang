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

#include "compiler.h"
#include "jitlib.h"

namespace X
{
	namespace Jit
	{
		class CppCompiler :
			public JitCompiler
		{
		public:
			CppCompiler();
			~CppCompiler();

			// Inherited via JitCompiler
			virtual bool Init(std::string& libFileName) override;
			virtual bool BuildCode(std::vector<std::string>& srcs, 
				std::vector<std::string>& exports, BuildCodeAction& action) override;
			virtual bool CompileAndLink(std::vector<std::string> srcs) override;
			virtual bool LoadLib() override;
			virtual void UnloadLib() override;
		protected:
			bool BuildFuncCode(bool isExternImpl, std::string& funcName, FuncInfo& funcInfo,
				std::string& code, std::string& stubCode, std::string& externalDeclstubCode);
			std::string MapDataType(std::string type, bool& isNativeObj);
			bool Build_VC(std::string strJitFolder, std::vector<std::string> srcs);
			bool Build_GCC(std::string strJitFolder, std::vector<std::string> srcs);
		};
	}
}