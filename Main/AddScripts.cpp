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
			std::vector<X::Value> passInParams;
			Hosting::I().RunAsBackend(fileName,it.second.code, passInParams);
		}
	}
}
