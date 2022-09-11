#pragma once

#define REGISTER_PACKAGE(pack_name,impl_pack_name)\
	X::g_pXHost->RegisterPackage(pack_name, [](X::XRuntime* rt)\
	{\
		impl_pack_name* pPackImpl = new impl_pack_name();\
		X::XPackage* pPackage = nullptr;\
		pPackImpl->Create(rt, &pPackage);\
		return pPackage;\
	});

#define REGISTER_OBJ_PACKAGE(pack_name,valObj)\
	X::g_pXHost->RegisterPackage(pack_name, valObj);

#define BEGIN_PACKAGE(class_name) \
		typedef class_name THIS_CLASS_NAME;\
		bool Create(X::XRuntime* rt, X::XPackage** ppackage)\
		{\
			std::vector<std::pair<int,X::XObj*>> _members_;\
			auto* pPackage = X::g_pXHost->CreatePackage(this);

#define DEF_U_FUNC_HEAD(fn_name)\
	{\
	int idx = pPackage->AddMethod(fn_name);\
	auto* pFuncObj = X::g_pXHost->CreateFunction(fn_name,\
	(X::U_FUNC)([](X::XRuntime* rt, X::XObj* pContext,\
		X::ARGS& params,\
		X::KWARGS& kwParams,\
		X::Value& retValue)\
		{\
			auto* pPackage = dynamic_cast<X::XPackage*>(pContext);\
			auto* pThis = (THIS_CLASS_NAME*)pPackage->GetEmbedObj();

#define DEF_U_FUNC_TAIL()\
			retValue = X::Value(_retVal);\
			return true;\
		}));\
	_members_.push_back(std::make_pair(idx, pFuncObj));\
	}


#define ADD_PROP2(prop_name_out,prop_name_inner)\
	{\
	int idx = pPackage->AddMethod(prop_name_out);\
	auto* pPropObj = X::g_pXHost->CreateProp(prop_name_out,\
	(X::U_FUNC)([](X::XRuntime* rt, X::XObj* pContext,\
		X::ARGS& params,\
		X::KWARGS& kwParams,\
		X::Value& retValue)\
		{\
			auto* pPackage = dynamic_cast<X::XPackage*>(pContext);\
			auto* pThis = (THIS_CLASS_NAME*)pPackage->GetEmbedObj();\
			pThis->prop_name_inner = params[0]; \
			return true; \
		}),\
	(X::U_FUNC)([](X::XRuntime* rt, X::XObj* pContext,\
		X::ARGS& params,\
		X::KWARGS& kwParams,\
		X::Value& retValue)\
		{\
			auto* pPackage = dynamic_cast<X::XPackage*>(pContext);\
			auto* pThis = (THIS_CLASS_NAME*)pPackage->GetEmbedObj();\
			retValue = X::Value(pThis->prop_name_inner);\
			return true; \
		})\
		);\
	_members_.push_back(std::make_pair(idx, pPropObj)); \
	}

#define ADD_PROP(prop_name) ADD_PROP2(#prop_name, prop_name)

#define ADD_FUNC0(fn_name,function_name,retType)\
	DEF_U_FUNC_HEAD(fn_name)\
	retType _retVal = pThis->function_name();\
	DEF_U_FUNC_TAIL()

#define ADD_FUNC1(fn_name,function_name,retType,p1)\
	DEF_U_FUNC_HEAD(fn_name)\
	retType _retVal = pThis->function_name((p1)params[0]);\
	DEF_U_FUNC_TAIL()

#define ADD_FUNC2(fn_name,function_name,retType,p1,p2)\
	DEF_U_FUNC_HEAD(fn_name)\
	retType _retVal = pThis->function_name((p1)params[0],(p2)params[1]);\
	DEF_U_FUNC_TAIL()

#define ADD_FUNC3(fn_name,function_name,retType,p1,p2,p3)\
	DEF_U_FUNC_HEAD(fn_name)\
	retType _retVal = pThis->function_name((p1)params[0],\
			(p2)params[1],(p3)params[2]);\
	DEF_U_FUNC_TAIL()



#define DEF_U_FUNC_XLANG_STYLE_CALL(function_name)\
	(X::U_FUNC)([](X::XRuntime* rt, X::XObj* pContext,\
		X::ARGS& params,\
		X::KWARGS& kwParams,\
		X::Value& retValue)\
		{\
			auto* pPackage = dynamic_cast<X::XPackage*>(pContext);\
			auto* pThis = (THIS_CLASS_NAME*)pPackage->GetEmbedObj();\
			return pThis->function_name(rt,pContext,params, kwParams, retValue);\
		})

#define ADD_FUNC(fn_name,function_name)\
	{\
	int idx = pPackage->AddMethod(fn_name);\
	auto* pFuncObj = X::g_pXHost->CreateFunction(fn_name,\
			DEF_U_FUNC_XLANG_STYLE_CALL(function_name)\
			);\
	_members_.push_back(std::make_pair(idx, pFuncObj));\
	}

#define DEF_CLASS_HEAD(class_name)\
	{\
	int idx= pPackage->AddMethod(class_name);\
	auto* pFuncObj = X::g_pXHost->CreateFunction(class_name,\
		(X::U_FUNC)([](X::XRuntime* rt, X::XObj* pContext,\
			X::ARGS& params,\
			X::KWARGS& kwParams,\
			X::Value& retValue)\
			{
#define DEF_CLASS_TAIL()\
				X::XPackage* pPackage_Srv = nullptr;\
				cls->Create(rt, &pPackage_Srv);\
				retValue = X::Value(pPackage_Srv);\
				return true;\
			}));\
	_members_.push_back(std::make_pair(idx, pFuncObj));\
	}

#define ADD_CLASS_INSTANCE(class_name,class_inst)\
	DEF_CLASS_HEAD(class_name)\
		auto* cls = class_inst;\
	DEF_CLASS_TAIL()

#define ADD_CLASS(class_name,class_impl_name)\
	DEF_CLASS_HEAD(class_name)\
		auto* cls = new class_impl_name(params,kwParams);\
	DEF_CLASS_TAIL()

#define END_PACKAGE \
	pPackage->Init((int)_members_.size());\
	for(auto& it:_members_)\
	{\
		X::Value v0(it.second);\
		pPackage->SetIndexValue(rt, nullptr, it.first, v0);\
	}\
	*ppackage = pPackage;\
	return true;\
	}

