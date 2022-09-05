#ifndef _X_LOAD_H_
#define _X_LOAD_H_
#include <vector>
#include <string>

namespace X
{
	enum class AppEventCode
	{
		Error,
		Exit,
		Continue,
	};
	struct Config
	{
		bool dbg = false;//-dbg
		bool enablePython = false;//-enable_python
		bool runAsBackend = false;//-run_as_backend
		bool enterEventLoop = false;//-event_loop
		bool runEventLoopInThread = false;
		std::string inlineCode;//-c "code"
		std::string fileName;
		std::vector<std::string> passInParams;
		std::string appPath;
		std::string xlangEnginePath;//for XLang engine dll path
		std::vector<std::string> dllSearchPath;
	};
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
		inline void SetXLangLibHandler(void* handle) { xlangLibHandler = handle; }
		inline void SetPythonLibHandler(void* handle) { pythonLibHandle = handle; }
		inline void SetDevopsLibHandler(void* handle) { devopsLibHandler = handle; }
		inline void* GetXLangLibHandler() { return xlangLibHandler; }
		inline void* GetPythonLibHandler() { return pythonLibHandle; }
		inline void* GetDevopsLibHandler() { return devopsLibHandler; }
	};
}

#endif //_X_LOAD_H_