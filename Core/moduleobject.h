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
			static void cleanup();
			Module* M() { return m_pModule; }
			virtual void GetBaseScopes(std::vector<Scope*>& bases) override;
			virtual const char* GetFileName() override
			{
				std::string str =  m_pModule ? m_pModule->GetModuleName() : "";
				return GetABIString(str);
			}
			virtual const char* GetPath() override
			{
				std::string str = m_pModule ? m_pModule->GetModulePath() : "";
				return GetABIString(str);
			}
			virtual int QueryMethod(const char* name, bool* pKeepRawParams = nullptr) override;
			virtual bool GetIndexValue(int idx, Value& v) override;

			// Inherited via Scope
			virtual Scope* GetParentScope() override;
		};
	}
}