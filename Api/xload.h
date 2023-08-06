#ifndef _X_LOAD_H_
#define _X_LOAD_H_
#include <string>

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
		inline Config& GetConfig() { return *m_pConfig; }
		int Load(Config* pCfg);
		void Unload();
		int Run();
		void EventLoop();
		inline void SetXLangLibHandler(void* handle) { xlangLibHandler = handle; }
		inline void SetPythonLibHandler(void* handle) { pythonLibHandle = handle; }
		inline void SetDevopsLibHandler(void* handle) { devopsLibHandler = handle; }
		inline void* GetXLangLibHandler() { return xlangLibHandler; }
		inline void* GetPythonLibHandler() { return pythonLibHandle; }
		inline void* GetDevopsLibHandler() { return devopsLibHandler; }
	};
}

#endif //_X_LOAD_H_