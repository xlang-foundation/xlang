/*
Copyright (C) 2024 The XLang Foundation
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

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
			static void Init();
			static void cleanup();
			Module* M() { return m_pModule; }
			virtual void GetBaseScopes(std::vector<Scope*>& bases) override;
			virtual const char* GetFileName() override
			{
				std::string str =  m_pModule ? m_pModule->GetModuleName() : "";
				return GetABIString(str);
			}
			virtual X::XRuntime* GetRT() override
			{
				return m_pModule ? (X::XRuntime*)m_pModule->GetRT() : nullptr;
			}
			virtual const char* GetPath() override
			{
				std::string str = m_pModule ? m_pModule->GetModulePath() : "";
				return GetABIString(str);
			}
			virtual int QueryMethod(const char* name, int* pFlags = nullptr) override;
			virtual bool GetIndexValue(int idx, Value& v) override;
			//return size of Module's member count
			virtual long long Size() override;
		};
	}
}