#pragma once
#include "singleton.h"
#include "module.h"
#include <vector>
#include "Locker.h"
#include <unordered_map>
namespace X
{
	class Hosting:
		public Singleton<Hosting>
	{
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
		AST::Module* Load(std::string& moduleName,
			const char* code, int size,unsigned long long& moduleKey);
		bool Run(unsigned long long moduleKey,X::KWARGS& kwParams,X::Value& retVal);
		bool Unload(AST::Module* pTopModule);
		bool Run(AST::Module* pTopModule,X::Value& retVal,
			bool stopOnEntry = false);
		bool Run(std::string& moduleName,
			const char* code, int size,X::Value& retVal);
		bool RunAsBackend(std::string& moduleName, std::string& code);
	};
}