#pragma once
#include <string>
#include <unordered_map>
#include "exp.h"
#include <vector>
#include "singleton.h"

namespace XPython {
	class Builtin :
		public Singleton<Builtin>
	{
		std::unordered_map<std::string, AST::ExternFunc*> m_mapFuncs;
	public:
		AST::ExternFunc* Find(std::string& name);
		bool Register(const char* name, void* func,
			std::vector<std::pair<std::string, std::string>>& params);
	};
}