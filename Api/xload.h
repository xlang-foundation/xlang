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

#ifndef _X_LOAD_H_
#define _X_LOAD_H_
#include <string>

#if !defined(FORCE_INLINE)
#if defined(_MSC_VER)
// Microsoft Visual C++ Compiler
#define FORCE_INLINE __forceinline
#elif defined(BARE_METAL) 
#define FORCE_INLINE inline
#elif defined(__GNUC__) || defined(__clang__)
// GCC or Clang Compiler
#define FORCE_INLINE __attribute__((always_inline)) inline
#else
// Fallback for other compilers
#define FORCE_INLINE inline
#endif
#endif

namespace X
{
	enum class AppEventCode
	{
		Error,
		Exit,
		Continue,
	};
	//don't use stl in this struct, will be used cross DLL or so, will cause problems
	struct Config
	{
		bool dbg = false;//-dbg
		bool enablePython = false;//-enable_python
		bool enablePythonDebug = false;//-enable_python_debug
		bool runAsBackend = false;//-run_as_backend
		bool enterEventLoop = false;//-event_loop
		bool runEventLoopInThread = false;
		bool padding1 = false;
		bool padding2 = false;
		bool padding3 = false;
		const char* inlineCode = nullptr;//-c "code"
		const char* fileName = nullptr;
		const char* passInParams = nullptr;
		const char* appPath = nullptr;
		const char* appFullName = nullptr;//include path and exe name
		const char* xlangEnginePath = nullptr;//for XLang engine dll path
		const char* dllSearchPath = nullptr;
		int dbgPort = 3142;
		~Config()
		{
			if (inlineCode) delete inlineCode;
			if (fileName) delete fileName;
			if (passInParams) delete passInParams;
			if (appPath) delete appPath;
			if (appFullName) delete appFullName;
			if (xlangEnginePath) delete xlangEnginePath;
			if (dllSearchPath) delete dllSearchPath;
		}
	};
//review 4/10/2023
//ABI issue with XLoad, different compiler may generate different layout of members
//TODO:change to use struct to wrap its members

	class XLoad
	{
		//context
		void* xlangLibHandler = nullptr;
		void* pythonLibHandle = nullptr;
		void* devopsLibHandler = nullptr;
		Config* m_pConfig = nullptr;
	public:
		XLoad();
		AppEventCode HandleAppEvent(int signum);
		FORCE_INLINE Config& GetConfig() { return *m_pConfig; }
		int Load(Config* pCfg);
		void Unload();
		int Run();
		void EventLoop();
		FORCE_INLINE void SetXLangLibHandler(void* handle) { xlangLibHandler = handle; }
		FORCE_INLINE void SetPythonLibHandler(void* handle) { pythonLibHandle = handle; }
		FORCE_INLINE void SetDevopsLibHandler(void* handle) { devopsLibHandler = handle; }
		FORCE_INLINE void* GetXLangLibHandler() { return xlangLibHandler; }
		FORCE_INLINE void* GetPythonLibHandler() { return pythonLibHandle; }
		FORCE_INLINE void* GetDevopsLibHandler() { return devopsLibHandler; }
	};
}

#endif //_X_LOAD_H_