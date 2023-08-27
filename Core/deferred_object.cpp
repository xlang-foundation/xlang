#include "deferred_object.h"
#include "function.h"
namespace X
{
	namespace Data
	{
		class DeferredObjectScope :
			virtual public AST::Scope
		{
			AST::StackFrame* m_stackFrame = nullptr;
		public:
			DeferredObjectScope() :
				Scope()
			{
				Init();
			}
			void clean()
			{
				if (m_stackFrame)
				{
					delete m_stackFrame;
					m_stackFrame = nullptr;
				}
			}
			~DeferredObjectScope()
			{
				if (m_stackFrame)
				{
					delete m_stackFrame;
				}
			}
			void Init()
			{
				m_stackFrame = new AST::StackFrame(this);
				m_stackFrame->SetVarCount(3);
				std::string strName;
				{
					strName = "load";
					auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
						X::ARGS& params,
						X::KWARGS& kwParams,
						X::Value& retValue)
					{
						auto* pObj = dynamic_cast<DeferredObject*>(pContext);
						bool bOK = pObj?pObj->Load(rt,params,kwParams):false;
						retValue = Value(bOK);
						return bOK;
					};
					X::U_FUNC func(f);
					AST::ExternFunc* extFunc = new AST::ExternFunc(strName, "load()", func);
					auto* pFuncObj = new X::Data::Function(extFunc);
					pFuncObj->IncRef();
					int idx = Scope::AddOrGet(strName, false);
					Value funcVal(pFuncObj);
					m_stackFrame->Set(idx, funcVal);
				}
			}
			// Inherited via Scope
			virtual int AddOrGet(std::string& name, bool bGetOnly, Scope** ppRightScope = nullptr) override
			{
				//can't add new members
				return Scope::AddOrGet(name, true, ppRightScope);
			}
			virtual Scope* GetParentScope() override
			{
				return nullptr;
			}
			virtual bool Set(XlangRuntime* rt, XObj* pContext, int idx, Value& v) override
			{
				m_stackFrame->Set(idx, v);
				return true;
			}
			virtual bool Get(XlangRuntime* rt, XObj* pContext, int idx, Value& v,
				LValue* lValue = nullptr) override
			{
				m_stackFrame->Get(idx, v, lValue);
				return true;
			}
		};
		static DeferredObjectScope* _deferredObjectScope = nullptr;
		void DeferredObject::GetBaseScopes(std::vector<AST::Scope*>& bases)
		{
			//after loaded, use real object's base scopes
			if (m_realObj.IsObject())
			{
				auto* pDataObj = dynamic_cast<Object*>(m_realObj.GetObj());
				pDataObj->GetBaseScopes(bases);
			}
			else//use deferred object's base scopes
			{
				bases.push_back(_deferredObjectScope);
				bases.push_back(this);
			}
		}
		void DeferredObject::Init()
		{
			_deferredObjectScope = new DeferredObjectScope();
		}
		void DeferredObject::cleanup()
		{
			if (_deferredObjectScope)
			{
				_deferredObjectScope->clean();
				delete _deferredObjectScope;
				_deferredObjectScope = nullptr;
			}
		}
		void DeferredObject::RestoreDeferredObjectContent(XlangRuntime* pXlRt,Object* pRealObj)
		{
			auto* pRealScope = dynamic_cast<AST::Scope*>(pRealObj);
			if (pRealScope)
			{
				for (auto it : m_Vars)
				{
					X::Value val;
					m_stackFrame->Get(it.second, val);
					std::string varName = it.first;
					int idx = pRealScope->AddOrGet(varName, true);
					if (idx >= 0)
					{
						if (val.IsObject() && val.GetObj()->GetType() == X::ObjType::DeferredObject)
						{
							auto* pSubDeferObject = dynamic_cast<DeferredObject*>(val.GetObj());
							X::Value subRealObj;
							pRealScope->Get(pXlRt, this, idx, subRealObj);
							if (subRealObj.IsObject())
							{
								auto* pRealSubObj = dynamic_cast<Object*>(subRealObj.GetObj());
								pSubDeferObject->m_realObj = subRealObj;
								pSubDeferObject->RestoreDeferredObjectContent(pXlRt, pRealSubObj);
							}
						}
						else 
						{
							pRealScope->Set(pXlRt, pRealObj, idx, val);
						}
					}
				}
			}
		}
		bool DeferredObject::Load(X::XRuntime* rt, X::ARGS& params, X::KWARGS& kwParams)
		{
			if (m_from_Import == nullptr)
			{
				return false;
			}
			X::Value realObj;
			auto* pXlRt = dynamic_cast<XlangRuntime*>(rt);
			bool bOK = m_from_Import->LoadModule(pXlRt,this, realObj, m_importInfo);
			if (!bOK)
			{
				return false;
			}
			m_realObj = realObj;
			//copy all properties from this object to real object
			//loop all variables in this object's scope and copy each value
			//to real object
			auto* pRealObj = dynamic_cast<Object*>(m_realObj.GetObj());
			RestoreDeferredObjectContent(pXlRt, pRealObj);
			return true;
		}
		AST::Scope* DeferredObject::GetParentScope()
		{
			if (m_realObj.IsObject())
			{
				auto* pObjScope = dynamic_cast<AST::Scope*>(m_realObj.GetObj());
				return pObjScope?pObjScope->GetParentScope():nullptr;
			}
			return nullptr;
		}
	}
}