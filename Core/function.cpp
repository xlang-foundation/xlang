#include "function.h"

namespace X
{
	namespace Data 
	{
		class FunctionOp :
			public AST::Scope
		{
			std::vector<X::Value> m_funcs;
		public:
			FunctionOp()
			{
				m_Vars =
				{
					{"getcode",0},
				};
				//API: getcode
				{
					std::string name("getcode");
					AST::ExternFunc* extFunc = new AST::ExternFunc(name,
						"retVal = getcode()",
						(X::U_FUNC)([](X::XRuntime* rt, XObj* pContext,
							ARGS& params,
							KWARGS& kwParams,
							X::Value& retValue)
							{
								Function* pFuncObj = dynamic_cast<Function*>(pContext);
								auto code = pFuncObj->GetFunc()->getcode(false);
								retValue = X::Value(code);
								return true;
							}));
					auto* pFuncObj = new Data::Function(extFunc, true);
					m_funcs.push_back(X::Value(pFuncObj));
				}
			}
			int QueryMethod(const char* name)
			{
				auto it = m_Vars.find(name);
				if (it != m_Vars.end())
				{
					return it->second;
				}
				else
				{
					return false;
				}
			}
			inline virtual bool Get(XlangRuntime* rt, XObj* pContext,
				int idx, X::Value& v, X::LValue* lValue = nullptr)
			{
				v = m_funcs[idx];
				return true;
			}
			void clean()
			{
				m_funcs.clear();
			}
			virtual Scope* GetParentScope()
			{
				return nullptr;
			}
		};
		static FunctionOp* _function_op = nullptr;
		void Function::cleanup()
		{
			if (_function_op)
			{
				_function_op->clean();
				delete _function_op;
				_function_op = nullptr;
			}
		}
		void Function::GetBaseScopes(std::vector<AST::Scope*>& bases)
		{
			Object::GetBaseScopes(bases);
			if (_function_op == nullptr)
			{
				_function_op = new FunctionOp();
			}
			bases.push_back(_function_op);
		}
		int Function::QueryMethod(const char* name, bool* pKeepRawParams)
		{
			return _function_op->QueryMethod(name);
		}
		bool Function::GetIndexValue(int idx, Value& v)
		{
			return _function_op->Get(nullptr, nullptr, idx, v);
		}
		Function::Function(AST::Func* p, bool bOwnIt)
		{
			m_ownFunc = bOwnIt;
			m_t = ObjType::Function;
			m_func = p;
		}
		Function::~Function()
		{
			if (m_ownFunc && m_func)
			{
				delete m_func;
			}
		}
		bool Function::Call(XRuntime* rt, XObj* pContext,
			ARGS& params,
			KWARGS& kwParams,
			X::Value& retValue)
		{
			return m_func->Call(rt, pContext, params, kwParams, retValue);
		}
		bool Function::CallEx(XRuntime* rt, XObj* pContext, 
			ARGS& params, KWARGS& kwParams, X::Value& trailer, X::Value& retValue)
		{
			return m_func->CallEx(rt, pContext, params, kwParams, trailer,retValue);
		}
	}
}