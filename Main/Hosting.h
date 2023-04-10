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
	public:
		void Cleanup()
		{
			if (m_pInteractiveModule)
			{
				m_pInteractiveModule->DecRef();
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
		AppEventCode HandleAppEvent(int signum);
		AST::Module* Load(const char* moduleName,
			const char* code, int size,unsigned long long& moduleKey);
		AST::Module* LoadWithScope(AST::Scope* pScope,const char* code, int size);
		X::Value NewModule();
		bool Run(unsigned long long moduleKey,X::KWARGS& kwParams,X::Value& retVal);
		bool InitRun(AST::Module* pTopModule,X::Value& retVal);
		bool RunFragmentInModule(AST::ModuleObject* pModuleObj,
			const char* code, int size, X::Value& retVal);
		bool RunCodeLine(const char* code, int size, X::Value& retVal);
		bool GetInteractiveCode(std::string& code);
		bool Unload(AST::Module* pTopModule);
		bool Run(AST::Module* pTopModule,X::Value& retVal,
			std::vector<std::string>& passInParams,
			bool stopOnEntry = false);
		bool Run(const char* moduleName,
			const char* code, int size,
			std::vector<std::string>& passInParams,
			X::Value& retVal);
		bool RunAsBackend(std::string& moduleName,std::string& code,
			std::vector<std::string>& passInParams);
		bool PostRunFragmentInMainThread(AST::ModuleObject* pModuleObj,std::string& code);
	};
}