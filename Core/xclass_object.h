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
			AST::VariableFrame* m_stackFrame = nullptr;
			std::vector<Value> m_bases;//this is diffrent with XClass's bases
			//will hold an instance per each Base class
		public:
			XClassObject()
			{
				m_t = ObjType::XClassObject;
				m_stackFrame = new AST::VariableFrame((AST::Scope*)this);
			}
			XClassObject(AST::XClass* p) :
				XClassObject()
			{
				m_obj = p;
				m_stackFrame->SetVarCount(p->GetVarNum());
				auto* pClassStack = p->GetClassStack();
				if (pClassStack)
				{
					m_stackFrame->Copy(pClassStack);
				}
			}
			inline std::vector<Value>& GetBases()
			{
				return m_bases;
			}
			void AssignClass(AST::XClass* p)
			{
				m_obj = p;
				m_stackFrame->SetVarCount(p->GetVarNum());
			}
			~XClassObject()
			{
				if (m_stackFrame)
				{
					delete m_stackFrame;
				}
			}
			inline virtual int GetBaseClassCount()
			{
				return (int)m_bases.size();
			}
			inline virtual X::Value GetBaseClass(int idx)
			{
				return m_bases[idx];
			}
			virtual bool ToBytes(XlangRuntime* rt, XObj* pContext, X::XLangStream& stream)
			{
				//TODO:check here
				AST::Expression exp;
				exp.SaveToStream(rt, pContext, m_obj, stream);
				stream << m_bases.size();
				for (auto& b : m_bases)
				{
					stream << b;
				}
				m_stackFrame->ToBytes(stream);
				return true;
			}
			virtual bool FromBytes(X::XLangStream& stream)
			{
				//pass this as XClass's Object
				auto* pPrevContext = stream.ScopeSpace().SetContext(this);
				AST::Expression exp;
				m_obj = exp.BuildFromStream<AST::XClass>(stream);
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
				m_stackFrame->FromBytes(stream);
				stream.ScopeSpace().SetContext(pPrevContext);
				return true;
			}
			virtual long long Size() override
			{
				return m_obj ? m_obj->GetVarNum() : 0;
			}
			inline virtual void GetBaseScopes(std::vector<AST::Scope*>& bases) override
			{
				Object::GetBaseScopes(bases);
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
				bases.push_back(pAst_Cls);
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
			inline XObj* QueryBaseObjForPackage(XObj* pPackage)
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
			inline AST::VariableFrame* GetStack()
			{
				return m_stackFrame;
			}
			inline AST::XClass* GetClassObj() { return m_obj; }
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