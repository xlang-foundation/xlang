#pragma once
#include <string>
#include <unordered_map>
#include "exp.h"
#include "func.h"
#include <vector>
#include "singleton.h"

namespace X {
	class Builtin :
		public Singleton<Builtin>
	{
		std::unordered_map<std::string, AST::ExternFunc*> m_mapFuncs;
	public:
		std::unordered_map<std::string, AST::ExternFunc*>& All()
		{
			return m_mapFuncs;
		}
		void Cleanup();
		AST::ExternFunc* Find(std::string& name);
		bool Register(const char* name, X::U_FUNC func,
			std::vector<std::pair<std::string, std::string>>& params,
			bool regToMeta=false);
		bool RegisterInternals();
	};
}