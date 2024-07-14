#include "moduleobject.h"
#include "function.h"
#include "Hosting.h"
#include "obj_func_scope.h"

namespace X
{
	namespace AST
	{
		static Obj_Func_Scope<2> _listScope;
		void ModuleObject::Init()
		{
			_listScope.Init();
			//API: runfragment
			{
				auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
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
				_listScope.AddFunc("runfragment", "runfragment(code)", f);
			}
			//API: setprimitive
			{
				std::string name("setprimitive");
				auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
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
				_listScope.AddFunc("setprimitive", "setprimitive(key,func)", f);
			}
			_listScope.Close();
		}
		void ModuleObject::cleanup()
		{
			_listScope.Clean();
		}
		void ModuleObject::GetBaseScopes(std::vector<Scope*>& bases)
		{
			Object::GetBaseScopes(bases);
			bases.push_back(_listScope.GetMyScope());
			bases.push_back(m_pModule->GetMyScope());
		}
		int ModuleObject::QueryMethod(const char* name, bool* pKeepRawParams)
		{
			std::string strName(name);
			int idx = _listScope.GetMyScope()->AddOrGet(strName,true);
			if (idx >= 0)
			{
				return -2-idx;//start from -2,then -3...
			}
			else
			{
				std::string strName(name);
				SCOPE_FAST_CALL_AddOrGet0(retIdx,m_pModule->GetMyScope(), strName, true);
				return retIdx;
			}
		}
		bool ModuleObject::GetIndexValue(int idx, Value& v)
		{
			if (idx <= -2)
			{
				_listScope.GetMyScope()->Get(nullptr, nullptr, -idx-2, v);
				return true;
			}
			else
			{
				m_pModule->GetStack()->Get(idx, v);
				return true;
			}
		}
	}
}