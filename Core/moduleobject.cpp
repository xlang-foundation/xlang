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
					auto f = [](X::XRuntime* rt, XObj* pContext,
						ARGS& params,
						KWARGS& kwParams,
						X::Value& retValue)
					{
						bool bPost = false;
						auto kwIt = kwParams.find("post");
						if (kwIt)
						{
							bPost = (bool)kwIt->val;
						}
						if (params.size() > 0)
						{
							std::string code;
							auto& v0 = params[0];
							std::string codePack;
							if (v0.IsObject()
								&& v0.GetObj()->GetType() == ObjType::Function)
							{
								auto* pFuncObj = dynamic_cast<X::Data::Function*>(v0.GetObj());
								code = pFuncObj->GetFunc()->GetCode();
							}
							else
							{
								code = v0.ToString();
							}
							auto* pModuleObj = dynamic_cast<ModuleObject*>(pContext);
							if (bPost)
							{
								Hosting::I().PostRunFragmentInMainThread(pModuleObj, code);
							}
							else
							{
								return Hosting::I().RunFragmentInModule(pModuleObj,
									code.c_str(), code.size(), retValue);
							}
						}
						else
						{
							retValue = X::Value(true);
							return true;
						}
					};
					X::U_FUNC func(f);
					AST::ExternFunc* extFunc = new AST::ExternFunc(name,
						"retVal = runfragment(code[,post = True|False])",
						func);
					auto* pFuncObj = new Data::Function(extFunc, true);
					m_funcs.push_back(X::Value(pFuncObj));
				}
				//API: setprimitive
				{
					std::string name("setprimitive");
					auto f = [](X::XRuntime* rt, XObj* pContext,
						ARGS& params,
						KWARGS& kwParams,
						X::Value& retValue)
					{
						if (params.size() == 2)
						{
							std::string name = params[0].ToString();
							auto* pModuleObj = dynamic_cast<ModuleObject*>(pContext);
							pModuleObj->M()->SetPrimitive(name, params[1], rt);
						}
						return true;
					};
					X::U_FUNC func(f);
					AST::ExternFunc* extFunc = new AST::ExternFunc(name,
						"setprimitive(name,obj)",func);
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
			int QueryMethod(const char* name)
			{
				auto it = m_Vars.find(name);
				if (it != m_Vars.end())
				{
					return it->second;
				}
				else
				{
					return -1;
				}
			}
			inline virtual bool Get(XlangRuntime* rt, XObj* pContext,
				int idx, X::Value& v, X::LValue* lValue = nullptr)
			{
				v = m_funcs[idx];
				return true;
			}
		};
		static ModuleOp* _module_op = nullptr;
		void ModuleObject::GetBaseScopes(std::vector<Scope*>& bases)
		{
			if (_module_op == nullptr)
			{
				_module_op = new ModuleOp();
			}
			Object::GetBaseScopes(bases);
			bases.push_back(_module_op);
			bases.push_back(m_pModule);
		}
		int ModuleObject::QueryMethod(const char* name, bool* pKeepRawParams)
		{
			int idx = _module_op->QueryMethod(name);
			if (idx >= 0)
			{
				return -2-idx;//start from -2,then -3...
			}
			else
			{
				std::string strName(name);
				return m_pModule->AddOrGet(strName, true);
			}
		}
		bool ModuleObject::GetIndexValue(int idx, Value& v)
		{
			if (idx <= -2)
			{
				return _module_op->Get(nullptr, nullptr, -idx-2, v);
			}
			else
			{
				return m_pModule->Get(nullptr, this, idx, v);
			}
		}
		void ModuleObject::cleanup()
		{
			if (_module_op)
			{
				_module_op->clean();
				delete _module_op;
				_module_op = nullptr;
			}
		}
		Scope* ModuleObject::GetParentScope()
		{
			return nullptr;
		}
	}
}