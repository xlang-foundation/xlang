#include <iostream>
#include <fstream>
#include "cppcompiler.h"
#include "port.h"
#include "md5.h"

#define  VS_MSVC_PATH "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat"
namespace X
{
	namespace Jit
	{
		CppCompiler::CppCompiler()
		{
		}

		CppCompiler::~CppCompiler()
		{
		}

		bool CppCompiler::Init(std::string& libFileName)
		{
			if (libFileName.size() == 0)
			{
				mJitFolder = mJitLib->Path() + Path_Sep + "_jit_";
				mLibFileName = mJitFolder + Path_Sep_S
					+ "bin" + Path_Sep_S + mJitLib->ModuleName() + ShareLibExt;

				std::string strJitSrcFolder = mJitFolder + Path_Sep + "src";
				_mkdir(mJitFolder.c_str());
				_mkdir(strJitSrcFolder.c_str());

			}
			else
			{
				mLibFileName = libFileName;
			}
			return true;
		}

		bool CppCompiler::BuildCode(std::vector<std::string>& srcs, 
			std::vector<std::string>& exports, BuildCodeAction& action)
		{
			static const char* stub_list_pat =
				"namespace X\n"
				"{\n"
				"	XHost* g_pXHost = nullptr;\n"
				"}\n"
				"\n"
				"extern \"C\"\n"
				"#if (WIN32)\n"
				"	__declspec(dllexport)\n"
				"#endif\n"
				"void Load(void* pHost, int** funcIdList, void*** funcs,const char*** hash_list,int* cnt)\n"
				"{\n"
				"	X::g_pXHost = (X::XHost*)pHost;\n"
				"	static void* __funcs__[%d] = \n"
				"	{\n"
				"		%s\n"
				"	};\n"
				"	static int __func_id_list__[%d] = \n"
				"	{\n"
				"		%s\n"
				"	};\n"
				"	static const char* __func_hash_list__[%d] = \n"
				"	{\n"
				"		%s\n"
				"	};\n"
				"	*funcIdList = __func_id_list__;\n"
				"	*funcs = __funcs__;\n"
				"	*hash_list = __func_hash_list__;\n"
				"	*cnt = %d;\n"
				"}\n";

				
				
			const int online_len = 1024 * 8;
			std::string allcode =
				"#include \"xlang.h\"\n";
			std::string otherIncludes;
			for (auto h : mIncludes)
			{
				otherIncludes += "#include \"" + h + "\"\n";
			}
			allcode += otherIncludes;
			allcode += "\n";

			std::string moduleName = mJitLib->ModuleName();
			std::string org_moduleName = moduleName;
			//replace space  and dot with _
			ReplaceAll(moduleName, " ", "_");
			ReplaceAll(moduleName, ".", "_");

			//Try to load lib with func's hash
			LoadLib();

			char funcLine[online_len];
			//namespace code
			std::string namespace_prefix;
			SPRINTF(funcLine, online_len, "\nnamespace %s{\n\n", moduleName.c_str());
			namespace_prefix = funcLine;

			std::string namespace_postfix;
			SPRINTF(funcLine, online_len, "\n}//namespace %s\n", moduleName.c_str());
			namespace_postfix = funcLine;
			allcode += namespace_prefix;

			std::string allStubCode;
			std::string allStubExternalImplDecl;

			std::string stub_list;
			std::string stub_funcId_list;
			std::string func_hash_list;
			bool needToGenFuncCodeFile = false;
			auto& funcList = mJitLib->GetFuncs();
			int funcCount = (int)funcList.size();
			int funcIndex = 0;
			bool bHashMatch = true;
			for (int funcIndex =0; funcIndex < funcCount; funcIndex++)
			{
				auto& funcInfo = funcList[funcIndex];
				if (funcInfo.langType != LangType::cpp)
				{
					continue;
				}
				std::string funcCode;
				std::string stubCode;
				std::string externalDeclstubCode;
				bool bOKToBuildBlockCode = BuildFuncCode(funcInfo.isExternImpl,
					funcInfo.name,funcInfo, funcCode, stubCode, externalDeclstubCode);
				if (!bOKToBuildBlockCode)
				{
					continue;
				}
				std::string funcHash = md5(funcCode);
				funcInfo.hash = funcHash;
				if(funcInfo.has_from_lib != funcHash)
				{
					bHashMatch = false;
				}
				std::string& funcName = funcInfo.name;
				if (funcInfo.isExternImpl)
				{
					allStubExternalImplDecl += externalDeclstubCode + ";\n";
					std::vector<std::string>& impl_srcs = funcInfo.externImplFileNameList;
					for (std::string& ss : impl_srcs)
					{
						srcs.push_back(ss);
					}
				}
				else
				{
					allcode += funcCode;
					needToGenFuncCodeFile = true;
				}
				allStubCode += stubCode;
				std::string strFunIndex = tostring((long)funcIndex);
				std::string stub_item = "(void*)" + moduleName + "::" + funcName + "_stub";
				if (stub_list.empty())
				{
					stub_list = stub_item;
					stub_funcId_list  = strFunIndex;
					func_hash_list = "\"" + funcHash + "\"";
				}
				else
				{
					stub_list += "," + stub_item;
					stub_funcId_list += "," + strFunIndex;
					func_hash_list += ",\"" + funcHash + "\"";
				}
				if (funcIndex>0 && (funcIndex%3) == 0)
				{
					stub_list += "\n\t\t";
				}
			}
			//if all hash match, then no need to build
			if (bHashMatch)
			{
				action = BuildCodeAction::HashAllMached;
				return true;
			}
			//Unload lib to make the compiler prcoess the new code
			UnloadLib();
			action = BuildCodeAction::NeedBuild;

			allcode += allStubCode;
			allcode += namespace_postfix;

			char stubListLine[online_len];
			SPRINTF(stubListLine, online_len, stub_list_pat, funcCount, 
				stub_list.c_str(), funcCount,
				stub_funcId_list.c_str(), funcCount,
				func_hash_list.c_str(),funcCount);
			allcode += stubListLine;
			//Write out code
			if (needToGenFuncCodeFile)
			{
				std::string strJitCpp = mJitFolder + Path_Sep + "src" + Path_Sep
					+ mJitLib->ModuleName() + ".cpp";
				std::ofstream file(strJitCpp);
				if (!file.is_open())
				{
					return false;
				}
				file.write((const char*)allcode.c_str(), allcode.length() * sizeof(char));
				file.close();

				srcs.push_back(strJitCpp);
			}
			return true;
		}
		bool CppCompiler::Build_VC(std::string strJitFolder, std::vector<std::string> srcs)
		{
#if (WIN32)
			//printf("Call Build_VC,strJitFolder=%s\n", strJitFolder.c_str());
			std::string clVarSet = VS_MSVC_PATH;
			std::string clVarSet_Quote = mJitLib->QuotePath(clVarSet);
			clVarSet_Quote += " x64 ";
			std::string initCmd = clVarSet_Quote + " && ";
			std::string clCmd;
			if (mJitLib->IsBuildWithDebug())
			{
				clCmd = "cl /Od /DWIN32 /D_USRDLL /D_WINDLL /Zi ";//for debug
			}
			else
			{
				clCmd = "cl /O2 /DWIN32 /D_USRDLL /D_WINDLL ";//for release
			}
			std::string includePath = mJitLib->GetXLangIncludePath();
			std::string xlangSrcs = includePath + "\\value.cpp";
			std::string includePath_I = "/I" + mJitLib->QuotePath(includePath);
			std::string srcCmd = " ";
			srcCmd += mJitLib->QuotePath(xlangSrcs);
			srcCmd += " ";
			for (auto src : srcs)
			{
				srcCmd += mJitLib->QuotePath(src);
				srcCmd += " ";
			}

			std::string objPath = strJitFolder + "\\obj\\";
			std::string binPath = strJitFolder + "\\bin\\";
			_mkdir(objPath.c_str());
			_mkdir(binPath.c_str());
			std::string binFileName = binPath + mJitLib->ModuleName() + ".dll";
			std::string objPath_Fo = " /Fo:" + mJitLib->QuotePath(objPath);
			std::string binPath_Fe = " /Fe:" + mJitLib->QuotePath(binFileName);

			std::string linkCmd = "/LD /link ";
			std::string buildLog = binPath + "build.txt";
			std::string logRedir = " > " + mJitLib->QuotePath(buildLog);
			std::string cmd = initCmd + clCmd + includePath_I + objPath_Fo + binPath_Fe + srcCmd + linkCmd + logRedir;

			SECURITY_ATTRIBUTES sa;
			HANDLE hNull;

			// Set the bInheritHandle flag so pipe handles are inherited
			sa.nLength = sizeof(SECURITY_ATTRIBUTES);
			sa.bInheritHandle = TRUE;
			sa.lpSecurityDescriptor = NULL;

			// Create a NULL device handle to redirect console output
			hNull = CreateFile("NUL", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE, &sa, OPEN_EXISTING, 0, NULL);
			if (hNull == INVALID_HANDLE_VALUE) 
			{
				std::cout << "Failed to create NULL device handle.\n";
				return 1;
			}

			STARTUPINFO StartupInfo;
			memset(&StartupInfo, 0, sizeof(STARTUPINFO));
			StartupInfo.cb = sizeof(STARTUPINFO);
			StartupInfo.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
			StartupInfo.wShowWindow = SW_SHOW;
			StartupInfo.hStdOutput = hNull;
			StartupInfo.hStdError = hNull;

			PROCESS_INFORMATION ProcessInformation;
			memset(&ProcessInformation, 0, sizeof(PROCESS_INFORMATION));
			ProcessInformation.hThread = INVALID_HANDLE_VALUE;
			ProcessInformation.hProcess = INVALID_HANDLE_VALUE;


			//have to ass /S and quote the whole command line
			std::string strCmd = "cmd /S /C \"" + cmd + "\"";
			//printf("run compiler\n%s\n", strCmd.c_str());
			BOOL bOK = CreateProcess(NULL, (LPSTR)strCmd.c_str(), NULL, NULL, TRUE,
				/*CREATE_NEW_CONSOLE*/0,//if use CREATE_NEW_CONSOLE will show new cmd window
				NULL, strJitFolder.c_str(),
				&StartupInfo, &ProcessInformation);
			if (bOK)
			{
				::WaitForSingleObject(ProcessInformation.hProcess, -1);
			}
#endif
			CloseHandle(hNull);
			return true;
		}
		bool CppCompiler::Build_GCC(std::string strJitFolder, std::vector<std::string> srcs)
		{
			//g++ -I /mnt/d/Dev/Galaxy/grus -shared -o libTest.so -fPIC jit_array_test.py.cpp  jit_array_test.py.stub.cpp ../extern_test.cpp ../cpp_folder/test2.cpp
#if (!WIN32)
			std::string strCmd = "g++";
			std::string clCmd = " -shared -fPIC ";
			if (mJitLib->IsBuildWithDebug())
			{
				clCmd += " -g ";//for debug
			}
			else
			{
			}
			std::string includePath = JITManager::I().GetPath();
			std::string includePath_I = " -I " + mJitLib->QuotePath(includePath);
			std::string srcCmd = " ";
			for (auto src : srcs)
			{
				srcCmd += mJitLib->QuotePath(src);
				srcCmd += " ";
			}

			std::string binPath = strJitFolder + "/bin/";
			_mkdir(binPath.c_str());
			std::string binFileName = binPath + mJitLib->LibFileName() + ".so";
			std::string binPath_Fe = " -o " + mJitLib->QuotePath(binFileName);

			std::string buildLog = binPath + "build.txt";
			std::string logRedir = " 2>&1 | tee " + mJitLib->QuotePath(buildLog);
			std::string cmd = clCmd + includePath_I + binPath_Fe + srcCmd + logRedir;

			unsigned long processId = 0;
			pid_t child_pid;
			child_pid = fork();
			if (child_pid >= 0)
			{
				const char* p1 = strCmd.c_str();
				const char* p2 = cmd.c_str();

				if (child_pid == 0)
				{//inside child process,run the executable
					std::string allcmd = strCmd + cmd;
					printf("run compiler\n%s\n", allcmd.c_str());
					char* argv[] = { "/bin/sh", "-c", (char*)allcmd.c_str(), 0 };
					//int retVal = execlp(argv[0], &argv[0], NULL);
					int retVal = execlp("/bin/sh", "/bin/sh", "-c", allcmd.c_str(), (char*)NULL);
					printf("execlp:%d\n", retVal);
				}
				else
				{//inside parent process, child_pid is the id for child process
					processId = child_pid;
					int status;
					waitpid((pid_t)processId, &status, WUNTRACED);
				}
			}
#endif
			return true;
		}

		bool CppCompiler::CompileAndLink(std::vector<std::string> srcs)
		{
			bool bOK = Build_VC(mJitFolder, srcs);
			return bOK;
		}

		bool CppCompiler::LoadLib()
		{
			bool bOK = false;
			if (mLibHandle == nullptr)
			{
				mLibHandle = LOADLIB(mLibFileName.c_str());
				if (mLibHandle)
				{
					std::string export_moduleName = "Load";

					Jit_Load_Proc loadProc = (Jit_Load_Proc)GetProc(mLibHandle, export_moduleName.c_str());
					if (loadProc)
					{
						int* funcIdList = nullptr;
						void** funcs = nullptr;
						const char** funcHashList = nullptr;
						int funcNum = 0;
						loadProc((void*)X::g_pXHost,&funcIdList,&funcs,&funcHashList,&funcNum);
						mJitLib->SetFuncStub(funcIdList, funcs, funcHashList,funcNum);
						bOK = true;
					}
				}
			}
			return bOK;
		}

		void CppCompiler::UnloadLib()
		{
			if (mLibHandle)
			{
				UNLOADLIB(mLibHandle);
				mLibHandle = nullptr;
			}
		}

		bool CppCompiler::BuildFuncCode(bool isExternImpl, std::string& funcName,
			FuncInfo& funcInfo, std::string& code,
			std::string& stubCode, std::string& externalDeclstubCode)
		{
			const int online_len = 2000;
			int paramSize = (int)funcInfo.params.size();
			std::string param_def;
			std::string param_in_stub;
			char line[online_len];
			for (int i = 0; i < paramSize; i++)
			{
				auto& paramInfo = funcInfo.params[i];
				bool isNativeObj = false;;
				auto mappedDataType = MapDataType(paramInfo.type, isNativeObj);
				param_def += "\t" + mappedDataType + " " + paramInfo.name;
				SPRINTF(line, online_len, "\t\t(%s)vars[%d]", mappedDataType.c_str(), i);
				param_in_stub += line;

				if (i < (paramSize - 1))
				{
					param_def += ",";
					param_in_stub += ",";
				}
				param_def += "\n";
				param_in_stub += "\n";
			}
			bool isNativeObj = false;
			std::string retType = MapDataType(funcInfo.retType, isNativeObj);
			std::string funcHead;
			//SPRINTF(line, online_len, "inline %s %s\n(\n", retType.c_str(), funcName.c_str());
			//TODO:if add inline, can't debug, so delete it, because use the O2 to compile
			SPRINTF(line, online_len, "%s %s\n(\n", retType.c_str(), funcName.c_str());
			funcHead = line;
			funcHead += param_def;
			funcHead += ")";
			std::string decl_func;
			decl_func = "extern " + funcHead;
			ReplaceAll(decl_func, "\n", "");
			ReplaceAll(decl_func, "\t", " ");
			if (isExternImpl)
			{
				externalDeclstubCode = decl_func;
				decl_func = "";
			}
			else
			{
				decl_func = "\t" + decl_func + ";\n";
			}
			funcHead += "\n{";
			std::string funcAll = funcHead + "\n" + funcInfo.code +
				"}\n";

			std::string funcStubHead;
			SPRINTF(line, online_len,
				"X::Value %s_stub(X::ARGS& vars)\n{\n"
				"%s"
				"\treturn %s(\n",
				funcName.c_str(),
				decl_func.c_str(),
				funcName.c_str()
			);
			funcStubHead = line;
			std::string funcStubAll = funcStubHead
				+ param_in_stub
				+ "\t\t);\n}\n";

			code = funcAll;
			stubCode = funcStubAll;
			return true;
		}

		std::string CppCompiler::MapDataType(std::string type, bool& isNativeObj)
		{
			isNativeObj = false;
			std::string strRetType;
			if (type == "" || type == "None")
			{
				strRetType = "void";
			}
			else if (type == "str")
			{
				strRetType = "std::string";
			}
			else if (type == "any")
			{
				strRetType = "X::Value";
			}
			else
			{
				strRetType = type;// use this format var:"c++ class name"
				isNativeObj = true;
			}
			return strRetType;
		}
	}
}