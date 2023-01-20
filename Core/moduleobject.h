#pragma once
#include "exp.h"
#include "object.h"
#include "stackframe.h"
#include "module.h"
#include "function.h"

namespace X
{
	namespace AST
	{
		enum class module_primitive
		{
			Output,
			Count,
		};
		class ModuleObject :
			virtual public XModule,
			virtual public Data::Object,
			virtual public Scope
		{
			Value m_primitives[(int)module_primitive::Count];
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
			void SetPrimitive(std::string& name, Value& valObj)
			{
				if (name == "Output")
				{
					m_primitives[(int)module_primitive::Output] = valObj;
				}
			}
			Value& GetPrimitive(module_primitive idx)
			{
				return m_primitives[(int)idx];
			}
			static void cleanup();
			Module* M() { return m_pModule; }
			virtual void GetBaseScopes(std::vector<Scope*>& bases) override;
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