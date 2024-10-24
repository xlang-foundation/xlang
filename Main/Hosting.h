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

#pragma once
#include "singleton.h"
#include "module.h"
#include <vector>
#include "Locker.h"
#include <unordered_map>
#include <algorithm>

namespace X
{
	namespace AST
	{
		class ModuleObject;
	}
	class Hosting:
		public Singleton<Hosting>
	{
		//use to run code line for interactive mode
		AST::Module* m_pInteractiveModule = nullptr;
		XlangRuntime* m_pInteractiveRuntime = nullptr;
		int m_ExeNum = -1;

		std::vector<AST::Module*> m_Modules;
		std::unordered_map<unsigned long long, AST::Module*> m_ModuleMap;
		Locker m_lock;

		unsigned long long AddModule(AST::Module* p)
		{
			auto key = (unsigned long long)p;
			m_lock.Lock();
			m_Modules.push_back(p);
			m_ModuleMap.emplace(std::make_pair(key,p));
			m_lock.Unlock();
			return key;
		}
		void RemoveModule(AST::Module* p)
		{
			m_lock.Lock();
			auto it = m_Modules.begin();
			while (it != m_Modules.end())
			{
				if (*it  == p)
				{
					m_Modules.erase(it);
					break;
				}
				else 
				{
					++it;
				}
			}
			auto mapIt = m_ModuleMap.find((unsigned long long)p);
			if (mapIt != m_ModuleMap.end())
			{
				m_ModuleMap.erase(mapIt);
			}
			m_lock.Unlock();
		}

		void SendBreakpointState(const std::string& md5, int line, int actualLine);
	public:
		void Cleanup()
		{
			if (m_pInteractiveModule)
			{
				//m_pInteractiveModule->DecRef();
				m_pInteractiveModule = nullptr;
			}
		}
		AST::Module* QueryModule(unsigned long long key)
		{
			AST::Module* pModule = nullptr;
			m_lock.Lock();
			auto mapIt = m_ModuleMap.find(key);
			if (mapIt != m_ModuleMap.end())
			{
				pModule = mapIt->second;
			}
			m_lock.Unlock();
			return pModule;
		}
		std::vector<AST::Module*> QueryModulesByPath(const std::string& path)
		{
			std::vector<AST::Module*> modules;
			m_lock.Lock();
			auto mapIt = m_ModuleMap.begin();
			while (mapIt != m_ModuleMap.end())
			{
				if (mapIt->second->GetModuleName() == path)
					modules.push_back(mapIt->second);
				++mapIt;
			}
			m_lock.Unlock();
			return modules;
		}
		std::vector<AST::Module*> QueryModulesByMd5(const std::string& strMd5)
		{
			std::vector<AST::Module*> modules;
			m_lock.Lock();
			auto mapIt = m_ModuleMap.begin();
			while (mapIt != m_ModuleMap.end())
			{
				if (mapIt->second->GetMd5() == strMd5)
					modules.push_back(mapIt->second);
				++mapIt;
			}
			m_lock.Unlock();
			return modules;
		}
		AppEventCode HandleAppEvent(int signum);
		AST::Module* Load(const char* moduleName,
			const char* code, int size,unsigned long long& moduleKey, const std::string& md5);
		AST::Module* LoadWithScope(AST::Scope* pScope,const char* code, int size);
		X::Value NewModule();
		bool Run(unsigned long long moduleKey,X::KWARGS& kwParams,X::Value& retVal);
		bool InitRun(AST::Module* pTopModule,X::Value& retVal);
		bool RunFragmentInModule(AST::ModuleObject* pModuleObj,	const char* code, int size, X::Value& retVal, int exeNum = -1);
		bool RunCodeLine(const char* code, int size, X::Value& retVal);
		bool GetInteractiveCode(std::string& code);
		int GetExeNum(){return m_ExeNum;};
		bool Unload(AST::Module* pTopModule);
		bool Run(AST::Module* pTopModule,X::Value& retVal,
			std::vector<X::Value>& passInParams,
			bool stopOnEntry = false,
			bool keepModuleWithRuntime = false,
			bool noDebug = false);
		bool Run(const char* moduleName,
			const char* code, int size,
			std::vector<X::Value>& passInParams,
			X::Value& retVal,bool noDebug = false);
		bool SimpleRun(const char* moduleName,
			const char* code, int size,
			X::Value& retVal);
		unsigned long long RunAsBackend(std::string& moduleName,std::string& code,std::vector<X::Value>& args);
		bool PostRunFragmentInMainThread(AST::ModuleObject* pModuleObj,std::string& code);
		void SetDebugMode(bool bDebug);
	};
}