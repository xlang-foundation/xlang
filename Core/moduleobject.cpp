#include "moduleobject.h"
#include "function.h"
#include "Hosting.h"

namespace X
{
	namespace AST
	{
		class ModuleOp :
			public Scope
		{
			std::vector<X::Value> m_funcs;
		public:
			ModuleOp()
			{
				m_Vars =
				{
					{"runfragment",0},
					{"setprimitive",1},
				};
				//API: runfragment
				{
					std::string name("runfragment");
					AST::ExternFunc* extFunc = new AST::ExternFunc(name,
						"retVal = runfragment(code)",
						(X::U_FUNC)([](X::XRuntime* rt, XObj* pContext,
							ARGS& params,
							KWARGS& kwParams,
							X::Value& retValue)
							{
								if (params.size() > 0)
								{
									std::string code = params[0].ToString();
									auto* pModuleObj = dynamic_cast<ModuleObject*>(pContext);
									return Hosting::I().RunFragmentInModule(pModuleObj,
										code.c_str(), code.size(), retValue);
								}
								else
								{
									retValue = X::Value(true);
									return true;
								}
							}));
					auto* pFuncObj = new Data::Function(extFunc, true);
					m_funcs.push_back(X::Value(pFuncObj));
				}
				//API: setprimitive
				{
					std::string name("setprimitive");
					AST::ExternFunc* extFunc = new AST::ExternFunc(name,
						"setprimitive(name,obj)",
						(X::U_FUNC)([](X::XRuntime* rt, XObj* pContext,
							ARGS& params,
							KWARGS& kwParams,
							X::Value& retValue)
							{
								if (params.size() == 2)
								{
									std::string name = params[0].ToString();
									auto* pModuleObj = dynamic_cast<ModuleObject*>(pContext);
									pModuleObj->M()->SetPrimitive(name, params[1],rt);
								}
								return true;
							}));
					auto* pFuncObj = new Data::Function(extFunc, true);
					m_funcs.push_back(X::Value(pFuncObj));
				}
			}
			void clean()
			{
				m_funcs.clear();
			}
			virtual Scope* GetParentScope()
			{
				return nullptr;
			}
			inline virtual bool Set(XlangRuntime* rt, XObj* pContext,
				int idx, X::Value& v)
			{
				return false;
			}

			inline virtual bool Get(XlangRuntime* rt, XObj* pContext,
				int idx, X::Value& v, X::LValue* lValue = nullptr)
			{
				v = m_funcs[idx];
				return true;
			}
		};
		static ModuleOp _module_op;
		void ModuleObject::GetBaseScopes(std::vector<Scope*>& bases)
		{
			bases.push_back(&_module_op);
			bases.push_back(m_pModule);
		}
		void ModuleObject::cleanup()
		{
			_module_op.clean();
		}
		Scope* ModuleObject::GetParentScope()
		{
			return nullptr;
		}
	}
}