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
			public virtual AST::Scope,
			public virtual Data::Object
		{
			//real object's create information
			//from Import
			AST::Import* m_from_Import = nullptr;
			AST::ImportInfo* m_importInfo = nullptr;

			//if this is true, means this object is a proxy
			//not root DeferredObject
			bool m_IsProxy = false;
			
			AST::VariableFrame* m_stackFrame = nullptr;
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
			ObjRef(),XObj(),Scope(), Object()
			{
				m_t = ObjType::DeferredObject;
				m_stackFrame = new AST::VariableFrame(this);
			}
			~DeferredObject()
			{
				delete m_stackFrame;
			}
			inline void SetImportInfo(AST::Import* pImport, AST::ImportInfo* pInfo)
			{
				m_from_Import = pImport;
				m_importInfo = pInfo;
			}
			bool Load(X::XRuntime* rt,X::ARGS& params,X::KWARGS& kwParams);

			// Inherited via Scope
			virtual Scope* GetParentScope() override;
			virtual int AddOrGet(std::string& name, bool bGetOnly, Scope** ppRightScope = nullptr) override
			{
				//if this is true, means this Var is right value
				//so we need to create a DeferredObject as its proxy
				//after this parent object loaded, will be assign to real object
				bool needSetProxy = false;
				if (bGetOnly)
				{
					needSetProxy = true;
				}
				bGetOnly = false;//deferred object can accespet all members request
				auto idx =  Scope::AddOrGet(name, bGetOnly, ppRightScope);
				if (idx>=0)
				{
					m_stackFrame->SetVarCount(GetVarNum());
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
			inline virtual bool Set(XlangRuntime* rt, XObj* pContext,
				int idx, X::Value& v) override
			{
				assert(idx != -1);
				m_stackFrame->Set(idx, v);
				return true;
			}

			inline virtual bool Get(XlangRuntime* rt, XObj* pContext,
				int idx, X::Value& v, LValue* lValue = nullptr) override
			{
				m_stackFrame->Get(idx, v, lValue);
				return true;
			}
		};
	}
}
