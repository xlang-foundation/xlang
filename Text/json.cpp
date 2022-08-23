#include "json.h"
#include "Hosting.h"
#include "utility.h"
#include "port.h"

//TODO: for security, diable any funtion call inside json string or file

namespace X
{
	bool JsonWrapper::LoadFromString(void* rt, void* pContext,
		ARGS& params,
		KWARGS& kwParams,
		X::Value& retValue)
	{
		if (params.size() == 0)
		{
			retValue = X::Value(false);
			return false;
		}
		std::string jsonStr = params[0].ToString();
		std::string fileName = "inline_code";
		bool bOK = X::Hosting::I().Run(fileName, jsonStr.c_str(),
			(int)jsonStr.size(), retValue);
		return bOK;
	}
	bool JsonWrapper::LoadFromFile(void* rt, void* pContext,
		ARGS& params,
		KWARGS& kwParams,
		X::Value& retValue)
	{
		if (params.size() == 0)
		{
			retValue = X::Value(false);
			return false;
		}
		std::string fileName = params[0].ToString();
		if (!IsAbsPath(fileName))
		{
			X::Runtime* pRt = (X::Runtime*)(X::XRuntime*)rt;
			std::string curPath = pRt->M()->GetModulePath();
			fileName = curPath + Path_Sep_S + fileName;
		}
		std::string jsonStr;
		bool bOK = LoadStringFromFile(fileName, jsonStr);
		if (!bOK)
		{
			retValue = X::Value(false);
			return false;
		}
		bOK = X::Hosting::I().Run(fileName, jsonStr.c_str(),
			(int)jsonStr.size(), retValue);
		return bOK;
	}
}