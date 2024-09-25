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
            cfg.enablePython = python_dbg;
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