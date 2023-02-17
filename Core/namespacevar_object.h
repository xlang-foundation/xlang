#pragma once
#include "object.h"
#include "scope.h"

namespace X
{
	namespace Data
	{
		class NamespaceVarObject :
			public virtual AST::Scope,
			public virtual Object
		{
		protected:
			
			// Inherited via Scope
			virtual AST::Scope* GetParentScope() override;
		};
	}
}
