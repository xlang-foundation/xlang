#include "json.h"
#include "Hosting.h"
#include "utility.h"
#include "port.h"

//TODO: for security, disable any function call inside json string or file

namespace X
{
	X::Value JsonWrapper::LoadFromString(std::string jsonStr)
	{
		std::string fileName = "inline_code";
		X::Value retValue;
		std::vector<std::string> passInParams;
		X::Hosting::I().Run(fileName, jsonStr.c_str(),
			(int)jsonStr.size(), passInParams,retValue);
		return retValue;
	}
	X::Value  JsonWrapper::LoadFromFile(X::XRuntime* rt, X::XObj* pContext,
		std::string fileName)
	{
		if (!IsAbsPath(fileName))
		{
			X::XlangRuntime* pRt = (X::XlangRuntime*)rt;
			std::string curPath = pRt->M()->GetModulePath();
			fileName = curPath + Path_Sep_S + fileName;
		}
		X::Value retValue;
		std::string jsonStr;
		bool bOK = LoadStringFromFile(fileName, jsonStr);
		if (!bOK)
		{
			retValue = X::Value(false);
		}
		std::vector<std::string> passInParams;
		X::Hosting::I().Run(fileName, jsonStr.c_str(),
			(int)jsonStr.size(), passInParams,retValue);
		return retValue;
	}
}