#include "namespacevar_object.h"
#include "namespace_var.h"

namespace X
{
	namespace Data
	{
		void NamespaceVarObject::GetBaseScopes(std::vector<AST::Scope*>& bases)
		{
			Object::GetBaseScopes(bases);
			bases.push_back(dynamic_cast<AST::Scope*>(this));
		}
	}
}