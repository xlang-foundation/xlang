#pragma once
#include "singleton.h"
#include "module.h"
#include <vector>
namespace X
{
	class Hosting:
		public Singleton<Hosting>
	{
		std::vector<AST::Module*> m_Modules;
	public:
		bool Run(const char* code, int size);
		bool RunAsBackend(const char* code, int size);
	};
}