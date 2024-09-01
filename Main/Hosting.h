#pragma once
#include "singleton.h"
#include "module.h"
#include <vector>
#include "Locker.h"
#include <unordered_map>
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
		int m_pInteractiveExeNum = -1;

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

		void SendBreakpointState(const std::string& path, int line, int actualLine);
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
		AppEventCode HandleAppEvent(int signum);
		AST::Module* Load(const char* moduleName,
			const char* code, int size,unsigned long long& moduleKey);
		AST::Module* LoadWithScope(AST::Scope* pScope,const char* code, int size);
		X::Value NewModule();
		bool Run(unsigned long long moduleKey,X::KWARGS& kwParams,X::Value& retVal);
		bool InitRun(AST::Module* pTopModule,X::Value& retVal);
		bool RunFragmentInModule(AST::ModuleObject* pModuleObj,
			const char* code, int size, X::Value& retVal);
		bool RunCodeLine(const char* code, int size, X::Value& retVal, int exeNum = -1);
		bool GetInteractiveCode(std::string& code);
		int GetInteractiveExeNum(){return m_pInteractiveExeNum;};
		bool Unload(AST::Module* pTopModule);
		bool Run(AST::Module* pTopModule,X::Value& retVal,
			std::vector<X::Value>& passInParams,
			bool stopOnEntry = false,bool keepModuleWithRuntime = false);
		bool Run(const char* moduleName,
			const char* code, int size,
			std::vector<X::Value>& passInParams,
			X::Value& retVal);
		bool SimpleRun(const char* moduleName,
			const char* code, int size,
			X::Value& retVal);
		unsigned long long RunAsBackend(std::string& moduleName,std::string& code,std::vector<X::Value>& args);
		bool PostRunFragmentInMainThread(AST::ModuleObject* pModuleObj,std::string& code);
		void SetDebugMode(bool bDebug);
	};
}