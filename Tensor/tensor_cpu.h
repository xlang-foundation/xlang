#pragma once

#include "xpackage.h"
#include "xlang.h"
#include "ops_mgt.h"
#include "function.h"

namespace X
{
	class CpuTensor
	{
	public:
		BEGIN_PACKAGE(CpuTensor)
			APISET().AddVarFunc("get", &CpuTensor::Get);
			APISET().AddFunc<0>("add", &CpuTensor::Add);
			APISET().AddFunc<0>("minus", &CpuTensor::Minus);
			APISET().AddFunc<0>("mul", &CpuTensor::Multiply);
			APISET().AddFunc<0>("div", &CpuTensor::Divide);
		END_PACKAGE
	public:
		CpuTensor()
		{
		}
		//xlang use this way, T is this Object
		//   T.get(tensor1,tensor2,...,ToCpu = true)
		//
		//pass 1: build execution plan for data flow graph
		// will be an array of funcation calls
		//pass 2: execute this plan
		bool Get(X::XRuntime* rt, X::XObj* pContext,
			ARGS& params,
			KWARGS& kwParams,
			X::Value& retValue)
		{
			return true;
		}
		X::Value Add()
		{
			std::string name = "add";
			AST::ExternFunc* extFunc = new AST::ExternFunc(name,
				"add",
				(X::U_FUNC)([](X::XRuntime* rt, XObj* pContext,
					X::ARGS& params,
					X::KWARGS& kwParams,
					X::Value& retValue)
					{
						return true;
					}));
			auto* pFuncObj = new X::Data::Function(extFunc);
			Value action(pFuncObj);
			return X::Data::OpsManager::I().CreateOp(action);
		}
		X::Value Minus()
		{
			Value action;
			return X::Data::OpsManager::I().CreateOp(action);
		}
		X::Value Multiply()
		{
			Value action;
			return X::Data::OpsManager::I().CreateOp(action);
		}
		X::Value Divide()
		{
			Value action;
			return X::Data::OpsManager::I().CreateOp(action);
		}
	};
}