#pragma once
#include "singleton.h"
#include <string>
#include <unordered_map>

namespace X
{
	class ScriptsManager :
		public Singleton<ScriptsManager>
	{
		struct ScriptInfo
		{
			std::string code;
			std::string mainFunc;
		};
		std::unordered_map<std::string, ScriptInfo> m_scripts;
	public:
		void Load();
		void Run();
	};
}