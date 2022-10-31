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
			virtual public XModule,
			virtual public Data::Object,
			virtual public Scope
		{
			Module* m_pModule = nullptr;//just a ref, don't call delete
		public:
			ModuleObject(Module* pModule) :
				XModule(),Data::Object(), Scope()
			{
				m_pModule = pModule;
				m_t = X::ObjType::ModuleObject;
			}
			~ModuleObject()
			{
			}
			Module* M() { return m_pModule; }
			inline virtual void GetBaseScopes(std::vector<AST::Scope*>& bases) override
			{
				bases.push_back(m_pModule);
			}
			virtual std::string GetFileName() override
			{
				return m_pModule ? m_pModule->GetModuleName() : "";
			}
			virtual std::string GetPath() override
			{
				return m_pModule ? m_pModule->GetModulePath() : "";
			}
			// Inherited via Scope
			virtual Scope* GetParentScope() override;
		};
	}
}