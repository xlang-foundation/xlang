#pragma once

#include "compiler.h"

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
			virtual bool BuildCode(std::string strJitFolder,
				std::vector<std::string>& srcs, std::vector<std::string>& exports) override;
			virtual bool CompileAndLink(std::string strJitFolder,
				std::vector<std::string> srcs, std::vector<std::string> exports) override;
			virtual bool LoadLib(const std::string& libFileName, JitHost* pHost) override;
			virtual void UnloadLib() override;
		protected:
			bool BuildLoadModuleCode(std::string strJitFolder,
				std::vector<std::string>& srcs, std::vector<std::string> exports);
			std::string GetExportModuleName(int moduleIndex);
			bool BuildFuncCode(bool isExternImpl, std::string& funcName, JitFuncInfo* pFuncInfo,
				std::string& code, std::string& stubCode, std::string& externalDeclstubCode);
			bool BuildFuncCode(bool isExternImpl, JitFuncInfo* pFuncInfo, std::string& funcCode,
				std::string& stubCode, std::string& externalDeclstubCode);
			bool BuildClassCode(bool isExternImpl, std::string& className, JitClassInfo* pClassInfo,
				std::string& code, std::string& stubCode, std::string& externalDeclstubCode,
				std::string& prop_method_name_list,
				std::string& class_stub_func_list);
			bool BuildClassFuncCode(unsigned int funcId, bool isExternImpl, ClassFuncInfo* pClassFuncInfo, JitClassInfo* pClassInfo,
				std::string& code, std::string& stubCode, std::string& externalDeclstubCode);
			bool BuildClassConstructorCode(bool isExternImpl, ClassFuncInfo* pClassFuncInfo, JitClassInfo* pClassInfo,
				std::string& code, std::string& stubCode, std::string& externalDeclstubCode);
			std::string MapDataType(std::string type, bool& isNativeObj);
#ifdef _OTHER_WAY //keep for reference 
			bool Build_VC(std::string strJitFolder, std::vector<std::string> srcs);
			bool Build_GCC(std::string strJitFolder, std::vector<std::string> srcs);
#endif
		};
	}
}