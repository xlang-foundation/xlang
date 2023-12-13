#pragma once

#include <map>
#include <vector>
#include <string>

namespace X
{
	namespace Jit
	{
		class JitLib;
		struct FuncParseInfo
		{
			std::vector<std::pair<std::string, std::string>> parameters;
			std::string retType;
			std::string body;
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
			virtual bool BuildCode(std::string strJitFolder,
				std::vector<std::string>& srcs, std::vector<std::string>& exports) = 0;
			virtual bool CompileAndLink(std::string strJitFolder,
				std::vector<std::string> srcs, std::vector<std::string> exports) = 0;
			virtual bool LoadLib(const std::string& libFileName, JitHost* pHost) = 0;
			virtual void UnloadLib() = 0;
			void AddIncludeFile(std::string strFile)
			{
				mIncludes.push_back(strFile);
			}
		protected:
			bool TranslateCode(std::string& funcName,
				std::string& code,
				FuncParseInfo& info);
			JitLib* mJitLib = nullptr;
			std::string mLibFileName;
			void* mLibHandle = nullptr;
			std::vector<std::string> mIncludes;
		};
	}
}