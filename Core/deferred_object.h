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
#include "object.h"
#include "scope.h"
#include "stackframe.h"
#include "xproxy.h"
#include "import.h"

namespace X
{
	namespace Data
	{
		//this object is used to wrap a deferred loaded object
		//for example in import package_name defered as pack
		class DeferredObject :
			public virtual XDeferredObject,
			public virtual Data::Object,
			public virtual X::XCustomScope
		{
			AST::Scope* m_pMyScopeProxy = nullptr;
			AST::Scope* m_pMyScopeHolder = nullptr;//used to store members

			//real object's create information
			//from Import
			AST::Import* m_from_Import = nullptr;
			AST::ImportInfo* m_importInfo = nullptr;

			//if this is true, means this object is a proxy
			//not root DeferredObject
			bool m_IsProxy = false;
			
			AST::StackFrame* m_stackFrame = nullptr;
			X::Value m_realObj;
			virtual void GetBaseScopes(std::vector<AST::Scope*>& bases) override;
			void RestoreDeferredObjectContent(XlangRuntime* pXlRt,Object* pRealObj);

			virtual bool ToBytes(XlangRuntime* rt, XObj* pContext, X::XLangStream& stream) override
			{
				bool bHaveImport = (m_from_Import != nullptr);
				stream << bHaveImport;
				if (bHaveImport)
				{
					AST::Expression exp;
					exp.SaveToStream(rt, pContext, m_from_Import, stream);
					stream << m_importInfo->type;
					stream << m_importInfo->name;
					stream << m_importInfo->alias;
					stream << m_importInfo->fileName;
					stream << m_importInfo->Deferred;
				}
				stream << m_IsProxy;
				stream << m_realObj;
				bool bHaveStackFrame = (m_stackFrame != nullptr);
				stream << bHaveStackFrame;
				if (bHaveStackFrame)
				{
					m_stackFrame->ToBytes(stream);
				}
				return true;
			}
			virtual bool FromBytes(X::XLangStream& stream) override
			{
				auto* pPrevContext = stream.ScopeSpace().SetContext(this);
				bool bHaveImport = false;
				stream >> bHaveImport;
				if (bHaveImport)
				{
					AST::Expression exp;
					m_from_Import = exp.BuildFromStream<AST::Import>(stream);
					AST::ImportInfo info;
					stream >> info.type;
					stream >> info.name;
					stream >> info.alias;
					stream >> info.fileName;
					stream >> info.Deferred;
					m_importInfo = m_from_Import->FindMatchedImportInfo(info);
				}
				stream >> m_IsProxy;
				stream >> m_realObj;
				bool bHaveStackFrame = false;
				stream >> bHaveStackFrame;
				if (bHaveStackFrame)
				{
					m_stackFrame->FromBytes(stream);
				}
				stream.ScopeSpace().SetContext(pPrevContext);
				return true;
			}
		public:
			static void Init();
			static void cleanup();

			DeferredObject() :
			ObjRef(),XObj(),Object()
			{
				m_t = ObjType::DeferredObject;
				m_stackFrame = new AST::StackFrame();
				m_pMyScopeProxy = new AST::Scope();
				m_pMyScopeProxy->SetType(AST::ScopeType::Custom);
				m_pMyScopeProxy->SetDynScope(static_cast<X::XCustomScope*>(this));

				m_pMyScopeHolder = new AST::Scope();
				m_pMyScopeHolder->SetType(AST::ScopeType::DeferredObject);	
				m_pMyScopeHolder->SetVarFrame(m_stackFrame);
			}
			~DeferredObject()
			{
				delete m_stackFrame;
				delete m_pMyScopeProxy;
				delete m_pMyScopeHolder;
			}
			FORCE_INLINE void SetImportInfo(AST::Import* pImport, AST::ImportInfo* pInfo)
			{
				m_from_Import = pImport;
				m_importInfo = pInfo;
			}
			bool Load(X::XRuntime* rt,X::ARGS& params,X::KWARGS& kwParams);

			virtual int AddOrGet(const char* name, bool bGetOnly) override
			{
				//if this is true, means this Var is right value
				//so we need to create a DeferredObject as its proxy
				//after this parent object loaded, will be assign to real object
				bool needSetProxy = false;
				if (bGetOnly)
				{
					needSetProxy = true;
				}
				std::string strName(name);
				AST::Scope* pRightScope = nullptr;
				SCOPE_FAST_CALL_AddOrGet(idx,m_pMyScopeHolder,strName, false, &pRightScope);
				if (idx>=0)
				{
					m_stackFrame->SetVarCount(m_pMyScopeHolder->GetVarNum());
					if (needSetProxy)
					{
						DeferredObject* pProxy = new DeferredObject();
						pProxy->m_IsProxy = true;
						X::Value proxy(pProxy);
						m_stackFrame->Set(idx, proxy);
					}
				}
				return idx;
			}
			virtual bool Set(int idx, X::Value& v) override
			{
				assert(idx != -1);
				m_stackFrame->Set(idx, v);
				return true;
			}

			virtual bool Get(int idx, X::Value& v, void* lValue = nullptr)
			{
				m_stackFrame->Get(idx, v,(X::LValue*)lValue);
				return true;
			}
		};
	}
}
