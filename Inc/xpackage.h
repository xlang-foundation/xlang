#pragma once
#include "xlang.h"
#include "xhost.h"

#define REGISTER_PACKAGE(pack_name,impl_pack_name)\
	X::g_pXHost->RegisterPackage(pack_name, [](X::XRuntime* rt)\
	{\
		impl_pack_name* pPackImpl = new impl_pack_name();\
		X::XPackage* pPackage = nullptr;\
		pPackImpl->Create(rt, &pPackage);\
		return pPackage;\
	});
#define BEGIN_PACKAGE(class_name) \
		typedef class_name THIS_CLASS_NAME;\
		bool Create(X::XRuntime* rt, X::XPackage** ppackage)\
		{\
			std::vector<std::pair<int,X::XFunc*>> name_funcs;\
			auto* pPackage = g_pXHost->CreatePackage(this);

#define ADD_FUNC(fn_name,function_name)\
	{\
	int idx = pPackage->AddMethod(fn_name);\
	auto* pFuncObj = g_pXHost->CreateFunction(fn_name,\
			(X::U_FUNC)([](X::XRuntime* rt, void* pContext,\
				ARGS& params,\
				KWARGS& kwParams,\
				X::Value& retValue)\
				{\
					auto* pXObj = g_pXHost->ConvertObjFromPointer(pContext);\
					auto* pPackage = dynamic_cast<X::XPackage*>(pXObj);\
					auto* pThis = (THIS_CLASS_NAME*)pPackage->GetEmbedObj();\
					return pThis->function_name(rt,pContext,params, kwParams, retValue);\
				}));\
	name_funcs.push_back(std::make_pair(idx, pFuncObj));\
	}

#define ADD_CLASS(class_name,class_impl_name)\
	{\
	int idx= pPackage->AddMethod(class_name);\
	auto* pFuncObj = g_pXHost->CreateFunction(class_name,\
		(X::U_FUNC)([](X::XRuntime* rt, void* pContext,\
			ARGS& params,\
			KWARGS& kwParams,\
			X::Value& retValue)\
			{\
				class_impl_name* cls = \
					new class_impl_name(params,kwParams);\
				X::XPackage* pPackage_Srv = nullptr;\
				cls->Create(rt, &pPackage_Srv);\
				retValue = X::Value(pPackage_Srv);\
				return true;\
			}));\
	name_funcs.push_back(std::make_pair(idx, pFuncObj));\
	}

#define END_PACKAGE \
	pPackage->Init((int)name_funcs.size());\
	for(auto& it:name_funcs)\
	{\
		X::Value v0(it.second);\
		pPackage->SetIndexValue(rt, nullptr, it.first, v0);\
	}\
	*ppackage = pPackage;\
	return true;\
	}

