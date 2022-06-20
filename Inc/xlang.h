#pragma once
#include "module.h"
#include "package.h"

#define REGISTER_PACKAGE(pack_name,impl_pack_name)\
	X::Manager::I().Register(pack_name, [](X::Runtime* rt)\
	{\
		impl_pack_name* pPackImpl = new impl_pack_name();\
		X::AST::Package* pPackage = nullptr;\
		pPackImpl->Create(rt, &pPackage);\
		return pPackage;\
	});
#define BEGIN_PACKAGE(class_name) \
		typedef class_name THIS_CLASS_NAME;\
		bool Create(Runtime* rt, AST::Package** ppackage)\
		{\
			std::vector<std::pair<int,Data::Function*>> name_funcs;\
			auto* pPackage = new AST::Package(this);

#define ADD_FUNC(fn_name,function_name)\
	{\
	std::string name(fn_name);\
	int idx = pPackage->AddOrGet(name, false);\
	AST::ExternFunc* extFunc =\
		new AST::ExternFunc(name,\
			(AST::U_FUNC)([](X::Runtime* rt, void* pContext,\
				ARGS& params,\
				KWARGS& kwParams,\
				X::AST::Value& retValue)\
				{\
					auto* pPackage = (AST::Package*)pContext;\
					auto* pThis = (THIS_CLASS_NAME*)pPackage->GetObject();\
					return pThis->function_name(rt,pContext,params, kwParams, retValue);\
				}));\
	auto* pFuncObj = new Data::Function(extFunc);\
	name_funcs.push_back(std::make_pair(idx, pFuncObj));\
	}

#define ADD_CLASS(class_name,class_impl_name)\
	{\
	std::string name(class_name);\
	int idx= pPackage->AddOrGet(name, false);\
	AST::ExternFunc* extFunc =\
	new AST::ExternFunc(name,\
		(AST::U_FUNC)([](X::Runtime* rt, void* pContext,\
			ARGS& params,\
			KWARGS& kwParams,\
			AST::Value& retValue)\
			{\
				class_impl_name* cls = \
					new class_impl_name(params,kwParams);\
				AST::Package* pPackage_Srv = nullptr;\
				cls->Create(rt, &pPackage_Srv);\
				retValue = AST::Value(pPackage_Srv);\
				return true;\
			}));\
	auto* pFuncObj =new Data::Function(extFunc);\
	name_funcs.push_back(std::make_pair(idx, pFuncObj));\
	}

#define END_PACKAGE \
	pPackage->Init((int)name_funcs.size());\
	for(auto& it:name_funcs)\
	{\
		AST::Value v0(it.second);\
		pPackage->Set(rt, nullptr, it.first, v0);\
	}\
	*ppackage = pPackage;\
	return true;\
	}

