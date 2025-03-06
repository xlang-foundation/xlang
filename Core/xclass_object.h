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

namespace X {
	namespace Data {
		class XClassObject :
			public virtual XLangClass,
			public virtual Object
		{
		protected:
			AST::XClass* m_obj = nullptr;
			AST::Scope* m_pMyScope = nullptr;//for hold instance properties
			//same as m_obj->GetMyScope(), but will have own stack frame
			AST::StackFrame* m_variableFrame = nullptr;
			std::vector<Value> m_bases;//this is diffrent with XClass's bases
			//will hold an instance per each Base class
		public:
			XClassObject()
			{
				m_t = ObjType::XClassObject;
				m_pMyScope = new AST::Scope();
				m_pMyScope->SetType(AST::ScopeType::Class);
			}
			XClassObject(AST::XClass* p) :
				XClassObject()
			{
				m_obj = p;

				m_pMyScope->CopyFrom(m_obj->GetMyScope());
				m_variableFrame = new AST::StackFrame(m_pMyScope);
				m_pMyScope->SetVarFrame(m_variableFrame);


				//use XClas's MyScope not Scope( parent scope)
				m_variableFrame->SetVarCount(m_obj->GetMyScope()->GetVarNum());
				auto* pClassStack = p->GetClassStack();
				if (pClassStack)
				{
					m_variableFrame->Copy(pClassStack);
				}
			}
			FORCE_INLINE std::vector<Value>& GetBases()
			{
				return m_bases;
			}
			void AssignClass(AST::XClass* p)
			{
				m_obj = p;
				if (m_variableFrame == nullptr)
				{
					m_variableFrame = new AST::StackFrame(m_pMyScope);
					m_pMyScope->SetVarFrame(m_variableFrame);
				}
				m_variableFrame->SetVarCount(m_obj->GetMyScope()->GetVarNum());
			}
			~XClassObject()
			{
				if (m_variableFrame)
				{
					delete m_variableFrame;
				}
			}
			FORCE_INLINE virtual int GetBaseClassCount() 
			{
				return (int)m_bases.size();
			}
			FORCE_INLINE virtual X::Value GetBaseClass(int idx) override 
			{
				return m_bases[idx];
			}
			FORCE_INLINE virtual X::Value GetXClassName() override
			{
				return m_obj->GetNameString();
			}
			virtual bool ToBytes(XlangRuntime* rt, XObj* pContext, X::XLangStream& stream)
			{
				AST::Scope* pOldClassScope = stream.ScopeSpace().GetCurrentClassScope();
				stream.ScopeSpace().SetCurrentClassScope(m_obj->GetMyScope());
				//Pack Bases first
				stream << m_bases.size();
				for (auto& b : m_bases)
				{
					stream << b;
				}

				AST::Expression exp;
				exp.SaveToStream(rt, pContext, m_obj, stream);
				m_variableFrame->ToBytes(stream);
				stream.ScopeSpace().SetCurrentClassScope(pOldClassScope);
				return true;
			}
			virtual bool FromBytes(X::XLangStream& stream)
			{
				size_t size = 0;
				stream >> size;
				//remove all bases maybe come from init
				m_bases.clear();
				for (size_t i = 0; i < size; i++)
				{
					X::Value b;
					stream >> b;
					m_bases.push_back(b);
				}

				//pass this as XClass's Object
				auto* pPrevContext = stream.ScopeSpace().SetContext(this);
				AST::Expression exp;
				m_obj = exp.BuildFromStream<AST::XClass>(stream);
				//Create m_variableFrame
				m_pMyScope->CopyFrom(m_obj->GetMyScope());
				m_variableFrame = new AST::StackFrame(m_pMyScope);
				m_pMyScope->SetVarFrame(m_variableFrame);
				m_variableFrame->FromBytes(stream);
				stream.ScopeSpace().SetContext(pPrevContext);
				return true;
			}
			virtual long long Size() override
			{
				return m_obj ? m_obj->GetMyScope()->GetVarNum() : 0;
			}
			FORCE_INLINE virtual void GetBaseScopes(std::vector<AST::Scope*>& bases) override
			{
				Object::GetBaseScopes(bases);
				//we put this class or class instance's scope at first
				//means instance or class's member will overide bases
				
				//use instance's scope replace|| bases.push_back(pAst_Cls->GetMyScope());
				bases.push_back(m_pMyScope);
				auto* pAst_Cls = GetClassObj();
				auto& cls_bases = pAst_Cls->GetBases();
				for (auto& v : cls_bases)
				{
					if (v.IsObject())
					{
						Data::Object* pRealObj = dynamic_cast<Data::Object*>(v.GetObj());
						pRealObj->GetBaseScopes(bases);
					}
				}
			}
			virtual List* FlatPack(XlangRuntime* rt, XObj* pContext,
				std::vector<std::string>& IdList, int id_offset,
				long long startIndex, long long count) override;
			virtual bool CalcCallables(XlangRuntime* rt, XObj* pContext,
				std::vector<AST::Scope*>& callables) override
			{
				return m_obj ? m_obj->CalcCallables(rt, pContext, callables) : false;
			}
			virtual const char* ToString(bool WithFormat = false)
			{
				char v[1000];
				snprintf(v, sizeof(v), "Class:%s@0x%llx",
					m_obj->GetNameString().c_str(), (unsigned long long)this);
				std::string retStr(v);
				return GetABIString(retStr);
			}
			FORCE_INLINE XObj* QueryBaseObjForPackage(XObj* pPackage)
			{
				XPackage* pXPackage = dynamic_cast<XPackage*>(pPackage);
				if (pXPackage == nullptr)
				{
					return nullptr;
				}
				XObj* pRetObj = nullptr;
				for (auto& v : m_bases)
				{
					if (v.IsObject())
					{
						auto* pXObj = v.GetObj();
						if (pXObj->GetType() == ObjType::Package)
						{
							XPackage* pXPackage_V = dynamic_cast<XPackage*>(pXObj);
							if (pXPackage_V->IsSamePackage(pXPackage))
							{
								pRetObj = pXObj;
								break;
							}
						}
					}
				}
				return pRetObj;

			}
			virtual bool Set(Value valIdx, X::Value& val) override
			{
				bool bOK = false;
				std::string name = valIdx.ToString();
				int idx = m_obj->AddOrGet(name, false);
				if (idx >= 0)
				{
					m_variableFrame->Set(idx, val);
					bOK = true;
				}
				return bOK;
			}
			FORCE_INLINE AST::StackFrame* GetStack()
			{
				return m_variableFrame;
			}
			FORCE_INLINE AST::XClass* GetClassObj() { return m_obj; }
			virtual bool Call(XRuntime* rt, XObj* pContext, ARGS& params,
				KWARGS& kwParams,
				X::Value& retValue)
			{
				return m_obj->Call((XlangRuntime*)rt, this,
					params, kwParams, retValue);
			}
		};
	}
}