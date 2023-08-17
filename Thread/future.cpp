#include "future.h"
#include "scope.h"
#include "function.h"
#include "glob.h"

namespace X
{
	namespace Data
	{
		class FutureScope :
			virtual public AST::Scope
		{
			AST::StackFrame* m_stackFrame = nullptr;
		public:
			FutureScope() :
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
			~FutureScope()
			{
				if (m_stackFrame)
				{
					delete m_stackFrame;
				}
			}
			void Init()
			{
				m_stackFrame = new AST::StackFrame(this);
				m_stackFrame->SetVarCount(2);

				std::string strName;
				{
					strName = "get";
					auto f = [](X::XRuntime* rt, XObj* pThis,XObj* pContext,
						X::ARGS& params,
						X::KWARGS& kwParams,
						X::Value& retValue)
					{
						bool bOK = false;
						if (params.size() > 0)
						{
							Future* pObj = dynamic_cast<Future*>(pContext);
							bOK = pObj->GetResult(retValue, params[0]);
						}
						else
						{
							retValue = Value(bOK);
						}
						return bOK;
					};
					X::U_FUNC func(f);
					AST::ExternFunc* extFunc = new AST::ExternFunc(strName,
						"get(timeout)",func);
					auto* pFuncObj = new Function(extFunc);
					pFuncObj->IncRef();
					int idx = AddOrGet(strName, false);
					Value funcVal(pFuncObj);
					m_stackFrame->Set(idx, funcVal);
				}
				{
					strName = "then";
					auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
						X::ARGS& params,
						X::KWARGS& kwParams,
						X::Value& retValue)
					{
						if (params.size() > 0)
						{
							Future* pObj = dynamic_cast<Future*>(pContext);
							pObj->SetThenProc(params[0]);
						}
						//still return this Future Object to setup chain calls
						retValue = X::Value(pContext);
						return true;
					};
					X::U_FUNC func(f);
					AST::ExternFunc* extFunc = new AST::ExternFunc(strName,
						"then(callable)",func);
					auto* pFuncObj = new Function(extFunc);
					pFuncObj->IncRef();
					int idx = AddOrGet(strName, false);
					Value funcVal(pFuncObj);
					m_stackFrame->Set(idx, funcVal);
				}
			}
			// Inherited via Scope
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
		static FutureScope* _FutureScope = nullptr;

		void Future::GetBaseScopes(std::vector<AST::Scope*>& bases)
		{
			if (_FutureScope == nullptr)
			{
				_FutureScope = new FutureScope();
			}
			Object::GetBaseScopes(bases);
			bases.push_back(_FutureScope);
		}
		void Future::cleanup()
		{
			if (_FutureScope)
			{
				_FutureScope->clean();
				delete _FutureScope;
				_FutureScope = nullptr;
			}
		}
		void Future::SetVal(X::Value& v)
		{
			std::vector<X::Value> thenProcs;
			Lock();
			m_GotVal = true;
			m_Val = v;
			thenProcs = m_thenProcs;
			for (auto* w : m_waits)
			{
				w->Release();
			}
			Unlock();
			for(auto& proc: thenProcs)
			{
				if (!proc.IsObject())
				{
					continue;
				}
				XlangRuntime* rt = X::G::I().Threading(nullptr);
				X::ARGS params(1);
				X::KWARGS kwargs;
				X::Value retValue;
				params.push_back(v);
				proc.GetObj()->Call(rt, this, params, kwargs, retValue);
			}
		}
		bool Future::GetResult(X::Value& retVal,int timeout)
		{
			bool bOK = false;
			Lock();
			if (m_GotVal)
			{
				retVal = m_Val;
				bOK = true;
			}
			Unlock();
			if (bOK)
			{
				return true;
			}
			XWait* pWait = new XWait();
			Lock();
			m_waits.push_back(pWait);
			Unlock();
			bOK = pWait->Wait(timeout);
			Lock();
			if (m_GotVal)
			{
				retVal = m_Val;
				bOK = true;
			}
			for (auto it = m_waits.begin(); it != m_waits.end();)
			{
				if (*it == pWait)
				{
					it = m_waits.erase(it);
					break;
				}
				else
				{
					++it;
				}
			}
			Unlock();
			delete pWait;
			return bOK;
		}
	}
}