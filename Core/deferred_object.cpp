#include "deferred_object.h"
#include "function.h"
#include "obj_func_scope.h"

namespace X
{
	namespace Data
	{
		static Obj_Func_Scope<1> _deferredObjectScope;
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
				bases.push_back(_deferredObjectScope.GetMyScope());
				bases.push_back(m_pMyScopeProxy);
			}
		}
		void DeferredObject::Init()
		{
			_deferredObjectScope.Init();
			{
				auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					X::ARGS& params,
					X::KWARGS& kwParams,
					X::Value& retValue)
				{
					auto* pObj = dynamic_cast<DeferredObject*>(pContext);
					bool bOK = pObj ? pObj->Load(rt, params, kwParams) : false;
					retValue = Value(bOK);
					return bOK;
				};
				_deferredObjectScope.AddFunc("load", "load()", f);
			}
			_deferredObjectScope.Close();
		}
		void DeferredObject::cleanup()
		{
			_deferredObjectScope.Clean();
		}
		void DeferredObject::RestoreDeferredObjectContent(XlangRuntime* pXlRt,Object* pRealObj)
		{
			auto* pRealScope = pRealObj->GetMyScope();
			if (pRealScope)
			{
				for (auto it : m_pMyScopeHolder->GetVarMap())
				{
					X::Value val;
					m_stackFrame->Get(it.second, val);
					std::string varName = it.first;
					SCOPE_FAST_CALL_AddOrGet0(idx,pRealScope,varName, false);
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
	}
}