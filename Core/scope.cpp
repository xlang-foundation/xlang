#include "scope.h"
#include "module.h"

namespace X
{
	namespace AST
	{
		std::string Scope::GetModuleName(Runtime* rt)
		{
			return rt->M()->GetModuleName();
		}
	}
}