#pragma once
#include "exp.h"
#include "object.h"
#include "stackframe.h"
#include "module.h"

namespace X
{
	namespace AST
	{
		class ModuleObject :
			virtual public Data::Object,
			virtual public Scope
		{
			Module* m_pModule = nullptr;
		public:
			ModuleObject(Module* pModule) :
				Data::Object(), Scope()
			{
				m_pModule = pModule;
				m_t = X::ObjType::ModuleObject;
			}
			~ModuleObject()
			{
			}
			Module* M() { return m_pModule; }
			// Inherited via Scope
			virtual Scope* GetParentScope() override;
		};
	}
}