#pragma once

#include <map>
#include <vector>
#include <string>

namespace X
{
	namespace Jit
	{
		class JitLib;
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
			virtual bool BuildCode(std::vector<std::string>& srcs, std::vector<std::string>& exports) = 0;
			virtual bool CompileAndLink(std::vector<std::string> srcs) = 0;
			virtual bool LoadLib(const std::string& libFileName) = 0;
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