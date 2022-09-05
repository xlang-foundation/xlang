#include "AddScripts.h"
#include "Hosting.h"
//#include "devops.x"

namespace X
{
	void ScriptsManager::Load()
	{
		//m_scripts.emplace(std::make_pair("Devops", ScriptInfo{ DevOps_Code,""}));
	}
	void ScriptsManager::Run()
	{
		for (auto it : m_scripts)
		{
			std::string fileName = it.first;
			Hosting::I().RunAsBackend(fileName,it.second.code);
		}
	}
}
