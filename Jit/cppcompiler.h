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