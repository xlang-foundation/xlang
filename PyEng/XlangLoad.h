#pragma once
#include <string>
#include "xlang.h"

namespace X
{
	namespace PyBind
	{
		struct ParamConfig
		{
			std::string appPath;
			std::string appName;
		};
		bool LoadXLangEngine(ParamConfig& paramConfig, std::string searchPath,
				bool dbg = false, bool python_dbg = false);
		void UnloadXLangEngine();
	}
}