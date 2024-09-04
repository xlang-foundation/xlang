#include "XlangLoad.h"
#include "utility.h"
namespace X {
    namespace PyBind {
        static X::XLoad g_xLoad;

        static X::Config* g_pCfg = nullptr;
        char* NewFromString(std::string& inStr)
        {
            char* pNewStr = new char[inStr.length() + 1];
            memcpy(pNewStr, inStr.data(), inStr.length() + 1);
            return pNewStr;
        }


        bool LoadXLangEngine(ParamConfig& paramConfig, std::string searchPath,
            bool dbg, bool python_dbg)
        {
            X::Config* pCfg = new X::Config();
            X::Config& cfg = *pCfg;
            cfg.appPath = NewFromString(paramConfig.appPath);
            cfg.dbg = dbg;
            cfg.enablePythonDebug = python_dbg;
            cfg.enablePython = true;
            cfg.enterEventLoop = false;//will call g_xLoad.EventLoop() later
            cfg.runEventLoopInThread = false;
            cfg.dllSearchPath = NewFromString(searchPath);
            int retCode = g_xLoad.Load(pCfg);
            if (retCode == 0)
            {
                g_xLoad.Run();
            }
            g_pCfg = pCfg;
            return retCode == 0;
        }
        void UnloadXLangEngine()
        {
            g_xLoad.Unload();
            if(g_pCfg)
            {
				delete g_pCfg;
				g_pCfg = nullptr;
			}
        }
    }
}