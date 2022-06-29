#pragma once
#include "singleton.h"
#include "module.h"
#include <vector>
#include "Locker.h"
namespace X
{
	enum class AppEventCode
	{
		Exit,
		Continue,
	};
	class Hosting:
		public Singleton<Hosting>
	{
		std::vector<AST::Module*> m_Modules;
		Locker m_lock;

		void AddModule(AST::Module* p)
		{
			m_lock.Lock();
			m_Modules.push_back(p);
			m_lock.Unlock();
		}
		void RemoveModule(AST::Module* p)
		{
			m_lock.Lock();
			auto it = m_Modules.begin();
			while (it != m_Modules.end())
			{
				// remove odd numbers
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
			m_lock.Unlock();
		}
	public:
		AppEventCode HandleAppEvent(int signum);
		bool Run(std::string& moduleName,const char* code, int size);
		bool RunAsBackend(std::string& moduleName,const char* code, int size);
	};
}