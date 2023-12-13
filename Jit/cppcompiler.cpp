#include <iostream>
#include <fstream>
#include "cppcompiler.h"
#include "jitlib.h"
#include "port.h"

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
				std::string strJitFolder = mJitLib->Path() + Path_Sep + "_jit_";
				mLibFileName = strJitFolder + Path_Sep_S
					+ "bin" + Path_Sep_S + mJitLib->LibFileName() + ShareLibExt;
			}
			else
			{
				mLibFileName = libFileName;
			}
			return true;
		}

		bool CppCompiler::BuildLoadModuleCode(std::string strJitFolder,
			std::vector<std::string>& srcs, std::vector<std::string> exports)
		{
			std::string stubFileHead =
				"#include \"Jit_Object.h\"\n\n"
				"JitHost * g_pHost = nullptr;\n\n";
			std::string  init_func_head =
				"extern \"C\"  JIT_EXPORT int InitJitLib(JitHost* pHost,void* context)\n"
				"{\n"
				"\tg_pHost = pHost;\n";
			static std::string init_func_post =
				"\treturn 0;\n"
				"}\n";

			const int online_len = 1000;
			const char* funcExternLineTemp = "namespace %s {extern void %s(JitHost* pHost,void* context);}\n";
			const char* funcCallLineTemp = "\t%s(pHost,context);\n";

			char funcLine[online_len];
			std::string externs;
			std::string calls;
			for (auto it : exports)
			{
				auto pos = it.find("::");
				if (pos != it.npos)
				{
					std::string nm = it.substr(0, pos);
					std::string func = it.substr(pos + 2);
					SPRINTF(funcLine, online_len, funcExternLineTemp, nm.c_str(), func.c_str());
					externs += funcLine;
				}
				SPRINTF(funcLine, online_len, funcCallLineTemp, it.c_str());
				calls += funcLine;
			}
			std::string allcode =
				stubFileHead +
				externs +
				init_func_head +
				calls +
				init_func_post;
			//Write out Load code
			std::string strJitLoadCpp = strJitFolder + Path_Sep + "src" + Path_Sep
				+ mJitLib->LibFileName() + ".init.cpp";
			std::ofstream sfile(strJitLoadCpp);
			if (!sfile.is_open())
			{
				return false;
			}
			sfile.write((const char*)allcode.c_str(), allcode.length() * sizeof(char));
			sfile.close();

			srcs.push_back(strJitLoadCpp);

			return true;
		}

		bool CppCompiler::BuildCode(std::string strJitFolder,
			std::vector<std::string>& srcs, std::vector<std::string>& exports)
		{
			const int online_len = 1024 * 8;
			std::string allcode =
				"#include \"value.h\"\n";
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

			std::string export_moduleName = GetExportModuleName(moduleIndex);
			exports.push_back(moduleName + "::" + export_moduleName);
			std::string init_func_lines;
			char funcLine[online_len];
			SPRINTF(funcLine, online_len, init_func_head_temp, export_moduleName.c_str(), org_moduleName.c_str());
			std::string init_func_head = funcLine;

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

			bool needToGenFuncCodeFile = false;
			for (auto it : mJitLib->FuncMap(moduleIndex))
			{
				JitFuncInfo* pFuncInfo = it.second.pFuncInfo;
				if (pFuncInfo == nullptr)
				{//when the python module not loaded,but the SharedLib (Compiled) loaded
				//will come to this case. TODO:
					continue;
				}
				if (pFuncInfo->Lang() != LangType::cpp)
				{
					continue;
				}
				bool isExternImpl = pFuncInfo->IsExternImpl();
				std::string funcCode;
				std::string stubCode;
				std::string externalDeclstubCode;
				std::string class_stub_func_list;
				std::string prop_method_name_list;
				bool bOKToBuildBlockCode = true;

				if (it.second.blockType == JitBlockType::Func)
				{
					bOKToBuildBlockCode = BuildFuncCode(isExternImpl, pFuncInfo, funcCode, stubCode, externalDeclstubCode);
				}
				else if (it.second.blockType == JitBlockType::Class)
				{
					bOKToBuildBlockCode = BuildClassCode(isExternImpl, pFuncInfo->Name(),
						(JitClassInfo*)pFuncInfo, funcCode, stubCode, externalDeclstubCode, prop_method_name_list, class_stub_func_list);
				}
				if (!bOKToBuildBlockCode)
				{
					continue;
				}

				std::string& funcName = pFuncInfo->Name();
				if (isExternImpl)
				{
					allStubExternalImplDecl += externalDeclstubCode + ";\n";
					std::vector<std::string> impl_srcs = pFuncInfo->GetExternImplFileName();
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
				if (it.second.blockType == JitBlockType::Func)
				{
					SPRINTF(funcLine, online_len, funcLineTemp,
						pFuncInfo->Hash().c_str(),
						funcName.c_str(), funcName.c_str());
				}
				else if (it.second.blockType == JitBlockType::Class)
				{
					JitClassInfo* pClassInfo = (JitClassInfo*)pFuncInfo;
					SPRINTF(funcLine, online_len, classLineTemp,
						prop_method_name_list.c_str(),
						class_stub_func_list.c_str(),
						pFuncInfo->Hash().c_str(),
						funcName.c_str(),
						pClassInfo->Props().size(),
						pClassInfo->Funcs().size());
				}
				init_func_lines += funcLine;
			}
			allcode += namespace_postfix;

			allStubCode = stubFileHead + allStubExternalImplDecl + namespace_prefix + allStubCode;

			allStubCode += init_func_head;
			allStubCode += init_func_lines;
			allStubCode += init_func_post;
			allStubCode += namespace_postfix;
			//printf("//------cpp code----------\n%s\n",allcode.c_str());
			//Write out code
			if (needToGenFuncCodeFile)
			{
				std::string strJitCpp = strJitFolder + Path_Sep + "src" + Path_Sep
					+ mJitLib->ModuleName(moduleIndex) + ".cpp";
				std::ofstream file(strJitCpp);
				if (!file.is_open())
				{
					return false;
				}
				file.write((const char*)allcode.c_str(), allcode.length() * sizeof(char));
				file.close();

				srcs.push_back(strJitCpp);
			}
			//Write out stub code
			std::string strJitStubCpp = strJitFolder + Path_Sep + "src" + Path_Sep
				+ mJitLib->ModuleName(moduleIndex) + ".stub.cpp";
			std::ofstream sfile(strJitStubCpp);
			if (!sfile.is_open())
			{
				return false;
			}
			sfile.write((const char*)allStubCode.c_str(), allStubCode.length() * sizeof(char));
			sfile.close();

			srcs.push_back(strJitStubCpp);
			return true;
		}
#ifdef _OTHER_WAY //keep for reference 
		bool CppCompiler::Build_VC(std::string strJitFolder, std::vector<std::string> srcs)
		{
#if (WIN32)
			//printf("Call Build_VC,strJitFolder=%s\n", strJitFolder.c_str());
			//"C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat";
			std::string clVarSet = JITManager::I().GetCompilerPath();
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
			std::string includePath = JITManager::I().GetPath();
			std::string includePath_I = "/I" + mJitLib->QuotePath(includePath);
			std::string srcCmd = " ";
			for (auto src : srcs)
			{
				srcCmd += mJitLib->QuotePath(src);
				srcCmd += " ";
			}

			std::string objPath = strJitFolder + "\\obj\\";
			std::string binPath = strJitFolder + "\\bin\\";
			_mkdir(objPath.c_str());
			_mkdir(binPath.c_str());
			std::string binFileName = binPath + mJitLib->LibFileName() + ".dll";
			std::string objPath_Fo = " /Fo:" + mJitLib->QuotePath(objPath);
			std::string binPath_Fe = " /Fe:" + mJitLib->QuotePath(binFileName);

			std::string linkCmd = "/LD /link ";
			std::string buildLog = binPath + "build.txt";
			std::string logRedir = " > " + mJitLib->QuotePath(buildLog);
			std::string cmd = initCmd + clCmd + includePath_I + objPath_Fo + binPath_Fe + srcCmd + linkCmd + logRedir;

			STARTUPINFO StartupInfo;
			memset(&StartupInfo, 0, sizeof(STARTUPINFO));
			StartupInfo.cb = sizeof(STARTUPINFO);
			StartupInfo.wShowWindow = SW_SHOW;
			PROCESS_INFORMATION ProcessInformation;
			memset(&ProcessInformation, 0, sizeof(PROCESS_INFORMATION));
			ProcessInformation.hThread = INVALID_HANDLE_VALUE;
			ProcessInformation.hProcess = INVALID_HANDLE_VALUE;

			//have to ass /S and quote the whole command line
			std::string strCmd = "cmd /S /C \"" + cmd + "\"";
			printf("run compiler\n%s\n", strCmd.c_str());
			BOOL bOK = CreateProcess(NULL, (LPSTR)strCmd.c_str(), NULL, NULL, TRUE,
				/*CREATE_NEW_CONSOLE*/0,//if use CREATE_NEW_CONSOLE will show new cmd window
				NULL, strJitFolder.c_str(),
				&StartupInfo, &ProcessInformation);
			if (bOK)
			{
				::WaitForSingleObject(ProcessInformation.hProcess, -1);
			}
#endif
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
#endif //_OTHER_WAY //keep for reference 

		bool CppCompiler::CompileAndLink(std::string strJitFolder,
			std::vector<std::string> srcs, std::vector<std::string> exports)
		{
			std::string curPath = mJitLib->Path();
			BuildLoadModuleCode(strJitFolder, srcs, exports);

			int Debug = mJitLib->IsBuildWithDebug() ? 1 : 0;
			std::string binPath = (std::string)PyJit::Object()["os.path.join"](strJitFolder, "bin");
			auto M_ccompiler = PyJit::Object::Import("distutils.ccompiler");
			auto compiler = M_ccompiler["new_compiler"]();
			auto M_sysconfig = PyJit::Object::Import("distutils.sysconfig");
			M_sysconfig["customize_compiler"](compiler);
			GalaxyJitPtr pp = PyJit::Object(mJitLib->LibFileName()).ref();
			std::string binFileName =
				(std::string)((PyJit::Object)compiler["shared_object_filename"]).Call(1, &pp,
					PyJit::Object(std::map <std::string, PyJit::Object>
			{
				{"output_dir", PyJit::Object(binPath)},
			}));

			PyJit::Object sources(srcs);
			std::string includePath = JITManager::I().GetPath();
			std::vector<std::string> incDirs{ includePath, curPath };
			auto otherDirs = mJitLib->IncludeDirs();
			for (auto d : otherDirs)
			{
				MakeOSPath(d);
				if (!IsAbsPath(d))
				{
					d = curPath + Path_Sep_S + d;
				}
				incDirs.push_back(d);
			}
			std::vector<PyJit::Tuple> macros
			{
		#if (WIN32)
				PyJit::Tuple(std::vector<std::string>{"WIN32","1"})
		#endif
			};
			std::map <std::string, PyJit::Object> kwargs_
			{
				  { "output_dir", PyJit::Object("obj") },
				  { "include_dirs",PyJit::Object(incDirs)},
				  { "macros",PyJit::Object(macros)},
				  { "debug",PyJit::Object(Debug)}
			};
			PyJit::Object kwargs(kwargs_);
			pp = sources.ref();
			//pp will be deleted by .Call
			auto objects = ((PyJit::Object)compiler["compile"]).Call(1, &pp, kwargs);
			GalaxyJitPtr pp2[2] = { objects.ref(),PyJit::Object(binFileName).ref() };
			PyJit::Object(compiler["link_shared_object"]).Call(2, pp2, PyJit::Object(
				std::map <std::string, PyJit::Object>{
					{"debug", PyJit::Object(Debug)},
					{ "output_dir", PyJit::Object("bin") },
					{ "target_lang",PyJit::Object("c++") },
					{ "export_symbols",PyJit::Object(std::vector<std::string>{"InitJitLib"}) }
			}));
			return true;
		}

		bool CppCompiler::LoadLib(const std::string& libFileName, JitHost* pHost)
		{
			if (!libFileName.empty())
			{
				mLibFileName = libFileName;
			}
			bool bOK = false;
			if (mLibHandle == nullptr)
			{
				mLibHandle = LOADLIB(mLibFileName.c_str());
				if (mLibHandle)
				{
					std::string export_moduleName = "InitJitLib";

					Jit_Init_Proc InitProc = (Jit_Init_Proc)GetProc(mLibHandle, export_moduleName.c_str());
					if (InitProc)
					{
						InitProc(pHost, (void*)mJitLib);
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

		std::string CppCompiler::GetExportModuleName(int moduleIndex)
		{
			std::string moduleName = mJitLib->ModuleName(moduleIndex);
			ReplaceAll(moduleName, ".", "_");
			std::string export_moduleName = "Init_" + moduleName;
			return export_moduleName;
		}

		bool CppCompiler::BuildFuncCode(bool isExternImpl, std::string& funcName,
			JitFuncInfo* pFuncInfo, std::string& code,
			std::string& stubCode, std::string& externalDeclstubCode)
		{
			const int online_len = 2000;

			FuncParseInfo info;
			if (!TranslateCode(pFuncInfo->Name(), pFuncInfo->Code(), info))
			{
				return false;
			}

			std::string param_def;
			std::string param_in_stub;
			char line[online_len];
			for (int i = 0; i < (int)info.parameters.size(); i++)
			{
				auto it0 = info.parameters[i];
				bool isNativeObj = false;;
				auto mappedDataType = MapDataType(it0.second, isNativeObj);
				param_def += "\t" + mappedDataType + " " + it0.first;
				SPRINTF(line, online_len, "\t\t(%s)objs[%d]", mappedDataType.c_str(), i);
				param_in_stub += line;

				if (i < int(info.parameters.size() - 1))
				{
					param_def += ",";
					param_in_stub += ",";
				}
				param_def += "\n";
				param_in_stub += "\n";
			}
			bool isNativeObj = false;
			std::string retType = MapDataType(info.retType, isNativeObj);
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
			std::string funcAll = funcHead + info.body +
				"}\n";

			std::string funcStubHead;
			SPRINTF(line, online_len,
				"GalaxyJitPtr %s_stub(GalaxyJitPtr vars)\n{\n"
				"%s"
				"\tPyJit::Object objs(vars,true);\n"
				"\treturn PyJit::Object(%s(\n",
				funcName.c_str(),
				decl_func.c_str(),
				funcName.c_str()
			);
			funcStubHead = line;
			std::string funcStubAll = funcStubHead
				+ param_in_stub
				+ "\t\t)\n\t);\n}\n";

			code = funcAll;
			stubCode = funcStubAll;
			return true;
		}

		bool CppCompiler::BuildFuncCode(bool isExternImpl, JitFuncInfo* pFuncInfo,
			std::string& funcCode, std::string& stubCode, std::string& externalDeclstubCode)
		{
			const int online_len = 2000;

			std::string param_def;
			std::string param_in_stub;
			char line[online_len];
			ClassFuncInfo& funcHeadInfo = pFuncInfo->FuncHead();
			for (int i = 0; i < (int)funcHeadInfo.parameters.size(); i++)
			{
				auto& varInfo = funcHeadInfo.parameters[i];
				bool isNativeObj = false;;
				auto mappedDataType = MapDataType(varInfo.type, isNativeObj);
				param_def += "\t" + mappedDataType + " " + varInfo.name;
				SPRINTF(line, online_len, "\t\t(%s)objs[%d]", mappedDataType.c_str(), i);
				param_in_stub += line;

				if (i < int(funcHeadInfo.parameters.size() - 1))
				{
					param_def += ",";
					param_in_stub += ",";
				}
				param_def += "\n";
				param_in_stub += "\n";
			}
			const char* funcName = pFuncInfo->Name().c_str();
			bool isNativeObj = false;
			std::string retType = MapDataType(funcHeadInfo.returnType, isNativeObj);
			std::string funcHead;
			//SPRINTF(line, online_len, "inline %s %s\n(\n", retType.c_str(), funcName.c_str());
			//TODO:if add inline, can't debug, so delete it, because use the O2 to compile
			SPRINTF(line, online_len, "%s %s\n(\n", retType.c_str(), funcName);
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
			std::string funcAll = funcHead + pFuncInfo->Code() +
				"\n}\n";

			std::string funcStubHead;
			SPRINTF(line, online_len,
				"GalaxyJitPtr %s_stub(GalaxyJitPtr vars)\n{\n"
				"%s"
				"\tPyJit::Object objs(vars,true);\n"
				"\treturn PyJit::Object(%s(\n",
				funcName,
				decl_func.c_str(),
				funcName
			);
			funcStubHead = line;
			std::string funcStubAll = funcStubHead
				+ param_in_stub
				+ "\t\t)\n\t);\n}\n";

			funcCode = funcAll;
			stubCode = funcStubAll;

			return true;
		}

		bool CppCompiler::BuildClassCode(bool isExternImpl,
			std::string& className,
			JitClassInfo* pClassInfo,
			std::string& code,
			std::string& stubCode,
			std::string& externalDeclstubCode,
			std::string& prop_method_name_list,
			std::string& class_stub_func_list)
		{
			const int online_len = 2000;
			char line[online_len];
			const char* szClassName = className.c_str();
			std::string nativeClassName = pClassInfo->NativeClassName();
			const char* szNativeClassName = nativeClassName.c_str();

			static const char* BEGIN_CLASS =
				"/*****BEGIN******%s*****************/\n";
			SPRINTF(line, online_len, BEGIN_CLASS, szClassName);
			stubCode += line;

			static const char* class_new_template =
				"static void* %s_new()\n"//className
				"{\n"
				"\treturn new %s()\n"//szNativeClassName
				"}\n";

			static const char* class_dealloc_template =
				"static void %s_dealloc(void* classInstance)\n"//className
				"{\n"
				"\tdelete (%s*)classInstance;\n"//szNativeClassName
				"}\n"
				"";

			static const char* class_serialize_template =
				"static void %s_serialize_stub(GalaxyJitPtr self,unsigned long long streamId,bool InputOrOutput)\n"//className
				"{\n"
				"\tPyJit::Native<%s> N(self);\n"//szNativeClassName
				"\tN.Get()->serialize(streamId,InputOrOutput);\n"
				"}\n";

			//New Func
			std::string newStub;
			//dealloc
			SPRINTF(line, online_len, class_dealloc_template, szClassName, szNativeClassName);
			std::string dealloc_stub = line;

			std::string serialize_stub;

			static const char* new_dealloc_stub_temp =
				"\t(unsigned long long)%s_new,(unsigned long long)%s_dealloc,nullptr\n";
			static const char* new_dealloc_serialize_stub_temp =
				"\t(unsigned long long)%s_new,(unsigned long long)%s_dealloc,(unsigned long long)%s_serialize_stub\n";

			SPRINTF(line, online_len, new_dealloc_stub_temp, szClassName, szClassName);
			class_stub_func_list = line;
			if (pClassInfo->support_serialization())
			{
				SPRINTF(line, online_len, new_dealloc_serialize_stub_temp, szClassName, szClassName, szClassName);
				class_stub_func_list = line;
			}
			else
			{
				SPRINTF(line, online_len, new_dealloc_stub_temp, szClassName, szClassName);
				class_stub_func_list = line;
			}
			prop_method_name_list = "\"new\",\"dealloc\",\"serialize\"\n";
			//Props
			static const char* prop_get_func_template =
				"static GalaxyJitPtr %s_get_%s(GalaxyJitPtr self,void* closure)\n"//className,propName
				"{\n"
				"\tPyJit::Native<%s> N(self,0x%x);\n"//szNativeClassName,secret code
				"\treturn PyJit::Object(N.Get()->get%s());\n"//propName
				"}\n";
			static const char* prop_set_func_template =
				"static int %s_set_%s(GalaxyJitPtr self,GalaxyJitPtr value)\n"//className,propName
				"{\n"
				"\tPyJit::Native<%s> N(self,0x%x);\n"//szNativeClassName,secret code
				"\tN.Get()->set%s(%sPyJit::Object(value,true));\n"//propName,(prop_type) or empty
				"\treturn 0;\n"
				"}\n";

			static const char* bind_prop_get_func_template =
				"static GalaxyJitPtr %s_get_%s(GalaxyJitPtr self,void* closure)\n"//className,propName
				"{\n"
				"\tPyJit::Native<%s> N(self,0x%x);\n"//szNativeClassName,secret code
				"\treturn PyJit::Object(N.Get()->%s);\n"//bindInfo
				"}\n";
			static const char* bind_prop_set_func_template =
				"static int %s_set_%s(GalaxyJitPtr self,GalaxyJitPtr value)\n"//className,propName
				"{\n"
				"\tPyJit::Native<%s> N(self,0x%x);\n"//szNativeClassName,secret code
				"\tN.Get()->%s = %sPyJit::Object(value,true);\n"//bindInfo,(prop_type) or empty
				"\treturn 0;\n"
				"}\n";
			static const char* prop_stub_temp =
				"\t\t\t,(unsigned long long)%s_get_%s,(unsigned long long)%s_set_%s\n";

			auto& props = pClassInfo->Props();
			std::string prop_stub_code;
			if (props.size() > 0)
			{
				prop_method_name_list += "\t\t/*props*/";
			}
			for (int i = 0; i < props.size(); i++)
			{
				auto& prop = props[i];
				const char* propName = prop.name.c_str();
				const char* bindInfo = prop.bindto.c_str();
				unsigned int secret_code = (1 << 24) | i;
				if (prop.bindto.size() > 0)
				{
					SPRINTF(line, online_len, bind_prop_get_func_template, szClassName, propName,
						szNativeClassName, secret_code, bindInfo);
				}
				else
				{
					SPRINTF(line, online_len, prop_get_func_template, szClassName, propName,
						szNativeClassName, secret_code, propName);
				}
				prop_stub_code += line;
				bool isNativeObj = false;
				std::string propType = MapDataType(prop.type, isNativeObj);
				if (!propType.empty())
				{
					propType = "(" + propType + ")";
				}
				secret_code = (2 << 24) | i;

				if (prop.bindto.size() > 0)
				{
					SPRINTF(line, online_len, bind_prop_set_func_template, szClassName, propName,
						szNativeClassName, secret_code, bindInfo, propType.c_str());
				}
				else
				{
					SPRINTF(line, online_len, prop_set_func_template, szClassName, propName,
						szNativeClassName, secret_code, propName, propType.c_str());
				}
				prop_stub_code += line;
				SPRINTF(line, online_len, prop_stub_temp, szClassName, propName, szClassName, propName);
				class_stub_func_list += line;

				prop_method_name_list += ",\"" + prop.name + "\"";
				if (i % 3 == 0)
				{
					prop_method_name_list += "\n\t\t";
				}
			}
			if (props.size() > 0)
			{
				prop_method_name_list += "\n";
			}
			if (pClassInfo->HaveInitFunc())
			{
				std::string func_code;
				BuildClassConstructorCode(isExternImpl, &pClassInfo->InitFuncInfo(),
					pClassInfo, func_code, newStub, externalDeclstubCode);
			}
			else
			{//use default one without parameters
				SPRINTF(line, online_len, class_new_template, szClassName, szClassName, szClassName);
				newStub = line;
			}
			if (pClassInfo->support_serialization())
			{//add serialization stub
				SPRINTF(line, online_len, class_serialize_template, szClassName, szNativeClassName);
				serialize_stub = line;
			}
			auto& funcs = pClassInfo->Funcs();
			std::string func_stub_code;
			static const char* func_stub_temp =
				"\t\t\t,(unsigned long long)%s_%s_stub\n";
			if (funcs.size() > 0)
			{
				prop_method_name_list += "\t\t/*methods*/";
			}

			for (int i = 0; i < funcs.size(); i++)
			{
				auto& f = funcs[i];
				std::string func_code;
				std::string stub_Code;
				std::string externalDeclstubCode;
				//TODO: for embeded code:func_code;
				BuildClassFuncCode(i, isExternImpl, &f, pClassInfo, func_code, stub_Code, externalDeclstubCode);
				func_stub_code += stub_Code;

				SPRINTF(line, online_len, func_stub_temp, szClassName, f.name.c_str());
				class_stub_func_list += line;
				prop_method_name_list += ",\"" + f.name + "\"";
			}

			static const char* END_CLASS =
				"/*****END********%s*****************/\n\n";
			SPRINTF(line, online_len, END_CLASS, szClassName);
			std::string endCode = line;
			stubCode +=
				newStub +
				dealloc_stub +
				serialize_stub +
				prop_stub_code +
				func_stub_code +
				endCode;
			return true;
		}

		bool CppCompiler::BuildClassFuncCode(unsigned int funcId, bool isExternImpl, ClassFuncInfo* pClassFuncInfo,
			JitClassInfo* pClassInfo, std::string& code, std::string& stubCode, std::string& externalDeclstubCode)
		{
			const int online_len = 2000;
			std::string param_def;
			std::string param_in_stub;
			char line[online_len];
			for (int i = 0; i < pClassFuncInfo->parameters.size(); i++)
			{
				auto& var = pClassFuncInfo->parameters[i];
				bool isNativeObj = false;
				auto mappedDataType = MapDataType(var.type, isNativeObj);
				if (isNativeObj)
				{
					if (mappedDataType == pClassInfo->Name())
					{
						mappedDataType = pClassInfo->NativeClassName();
					}
					mappedDataType += "*";//change to pointer for native object
				}
				param_def += "\t" + mappedDataType + " " + var.name;
				SPRINTF(line, online_len, "\t\t(%s)objs[%d]", mappedDataType.c_str(), i);
				param_in_stub += line;

				if (i < int(pClassFuncInfo->parameters.size() - 1))
				{
					param_def += ",";
					param_in_stub += ",";
				}
				param_def += "\n";
				param_in_stub += "\n";
			}
			bool isNativeObj = false;
			bool isRetEmpty = false;
			if ((pClassFuncInfo->returnType == "") || (pClassFuncInfo->returnType == "None"))
			{
				isRetEmpty = true;
			}
			std::string retType = MapDataType(pClassFuncInfo->returnType, isNativeObj);
			std::string funcHead;
			//SPRINTF(line, online_len, "inline %s %s\n(\n", retType.c_str(), funcName.c_str());
			//TODO:if add inline, can't debug, so delete it, because use the O2 to compile
			SPRINTF(line, online_len, "%s %s\n(\n", retType.c_str(), pClassFuncInfo->name.c_str());
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
			std::string funcAll = funcHead + /*info.body +*/ "}\n";

			unsigned int secret_code = (3 << 24) | funcId;
			if (!isNativeObj)
			{
				if (isRetEmpty)
				{
					SPRINTF(line, online_len,
						"GalaxyJitPtr %s_%s_stub(GalaxyJitPtr self,GalaxyJitPtr vars)\n{\n"
						"\tPyJit::Native<%s> N(self,0x%x);\n"//className,secret code
						"\tPyJit::Object objs(vars,true);\n"
						"\tN.Get()->%s(\n%s\t);\n"
						"\treturn PyJit::None();\n"
						"}\n",
						pClassInfo->Name().c_str(),
						pClassFuncInfo->name.c_str(),
						pClassInfo->NativeClassName().c_str(), secret_code,
						pClassFuncInfo->name.c_str(),
						param_in_stub.c_str()
					);
				}
				else
				{
					SPRINTF(line, online_len,
						"GalaxyJitPtr %s_%s_stub(GalaxyJitPtr self,GalaxyJitPtr vars)\n{\n"
						"\tPyJit::Native<%s> N(self,0x%x);\n"//className,secret code
						"\tPyJit::Object objs(vars,true);\n"
						"\tPyJit::Object retObj(N.Get()->%s(\n%s\t\t)\n\t);\n"
						"\treturn retObj;\n"
						"}\n",
						pClassInfo->Name().c_str(),
						pClassFuncInfo->name.c_str(),
						pClassInfo->NativeClassName().c_str(), secret_code,
						pClassFuncInfo->name.c_str(),
						param_in_stub.c_str()
					);
				}
			}
			else
			{
				if (retType == pClassInfo->Name())
				{
					retType = pClassInfo->NativeClassName();
				}
				const char* className = pClassInfo->Name().c_str();
				const char* funcName = pClassFuncInfo->name.c_str();
				SPRINTF(line, online_len,
					"GalaxyJitPtr %s_%s_stub(GalaxyJitPtr self,GalaxyJitPtr vars)\n{\n"//className,funcname
					"\tPyJit::Native<%s> N(self,0x%x);\n"//className,secret code
					"\tPyJit::Object objs(vars,true);\n"
					"\t%s* pNativeObj = N.Get()->%s(\n%s\t);\n"//retType,funcname,param_in_stub
					"\treturn PyJit::Extract<%s,false>(self,\"%s\",pNativeObj);\n"//retType,retType
					"}\n",
					className, funcName,
					pClassInfo->NativeClassName().c_str(), secret_code,
					retType.c_str(), funcName, param_in_stub.c_str(),
					retType.c_str(), retType.c_str()
				);
			}
			stubCode = line;

			//code = funcAll;

			return true;
		}
		bool CppCompiler::BuildClassConstructorCode(bool isExternImpl, ClassFuncInfo* pClassFuncInfo,
			JitClassInfo* pClassInfo, std::string& code, std::string& stubCode, std::string& externalDeclstubCode)
		{
			const int online_len = 2000;
			std::string param_def;
			std::string param_in_stub;
			char line[online_len];
			for (int i = 0; i < pClassFuncInfo->parameters.size(); i++)
			{
				auto& var = pClassFuncInfo->parameters[i];
				bool isNativeObj = false;;
				auto mappedDataType = MapDataType(var.type, isNativeObj);
				param_def += "\t" + mappedDataType + " " + var.name;
				SPRINTF(line, online_len, "\t\t(%s)objs[%d]", mappedDataType.c_str(), i);
				param_in_stub += line;

				if (i < int(pClassFuncInfo->parameters.size() - 1))
				{
					param_def += ",";
					param_in_stub += ",";
				}
				param_def += "\n";
				param_in_stub += "\n";
			}
			std::string funcHead;
			//SPRINTF(line, online_len, "inline %s %s\n(\n", retType.c_str(), funcName.c_str());
			//TODO:if add inline, can't debug, so delete it, because use the O2 to compile
			SPRINTF(line, online_len, "%s\n(\n", pClassInfo->Name().c_str());
			funcHead = line;
			funcHead += param_def;
			funcHead += ")";
			std::string decl_func;
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
			std::string funcAll = funcHead + /*info.body +*/ "}\n";

			const char* className = pClassInfo->Name().c_str();
			std::string strNativeClassName = pClassInfo->NativeClassName();
			const char* nativeClassName = strNativeClassName.c_str();
			SPRINTF(line, online_len,
				"void* %s_new(GalaxyJitPtr self,GalaxyJitPtr vars)\n"//classname
				"{\n"
				"\tPyJit::Object objs(vars,true);\n"
				"\t%s* nativeObj = nullptr;\n"//nativeClassName
				"\tif(objs.GetCount()==0)\n"
				"\t{\n"
				"\t\tnativeObj =  new %s();\n"//nativeClassName
				"\t}\n"
				"\telse\n"
				"\t{\n"
				"\t\tnativeObj =  new %s(\n%s\t);\n"//nativeClassName,param_in_stub
				"\t}\n"
				"\treturn nativeObj;\n"
				"}\n",
				className,
				nativeClassName, nativeClassName, nativeClassName,
				param_in_stub.c_str()
			);
			stubCode = line;

			//code = funcAll;

			return true;
		}

		std::string CppCompiler::MapDataType(std::string type, bool& isNativeObj)
		{
			isNativeObj = false;
			std::string strRetType;
			if (type == "" || type == "None")
			{
				strRetType = "PyJit::Object";
			}
			else if (type == "bool" || type == "<class 'bool'>")
			{
				strRetType = "bool";
			}
			else if (type == "int" || type == "<class 'int'>")
			{
				strRetType = "int";
			}
			else if (type == "float" || type == "<class 'float'>")
			{//python's float is c's double
				strRetType = "double";
			}
			else if (type == "str" || type == "<class 'str'>")
			{
				strRetType = "std::string";
			}
			else if (type == "<built-in function any>")
			{
				strRetType = "";
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