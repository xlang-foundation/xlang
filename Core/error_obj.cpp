#include "error_obj.h"
#include "scope.h"
#include "obj_func_scope.h"

namespace X
{
	namespace Data
	{
		static Obj_Func_Scope<2> _errorScope;
		void Error::Init()
		{
			_errorScope.Init();
			{
				auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					ARGS& params,
					KWARGS& kwParams,
					X::Value& retValue)
					{
						auto* pObj = dynamic_cast<Object*>(pContext);
						auto* pErrorObj = dynamic_cast<Error*>(pObj);
						retValue = X::Value(pErrorObj->GetCode());
						return true;
					};
				_errorScope.AddFunc("getCode", "obj.getCode()", f);
			}
			{
				auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					ARGS& params,
					KWARGS& kwParams,
					X::Value& retValue)
					{
						auto* pObj = dynamic_cast<Object*>(pContext);
						auto* pErrorObj = dynamic_cast<Error*>(pObj);
						std::string strInfo = pErrorObj->GetInfo();
						retValue = X::Value(strInfo);
						return true;
					};
				_errorScope.AddFunc("getInfo", "obj.getInfo()", f);
			}
		}
		void Error::cleanup()
		{
			_errorScope.Clean();
		}
		void Error::GetBaseScopes(std::vector<AST::Scope*>& bases)
		{
			bases.push_back(_errorScope.GetMyScope());
		}
	}
}