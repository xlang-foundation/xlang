/*
Copyright (C) 2024 The XLang Foundation
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "json.h"
#include "Hosting.h"
#include "utility.h"
#include "port.h"

//TODO: for security, disable any function call inside json string or file

namespace X
{
	X::Value JsonWrapper::LoadFromString(std::string jsonStr)
	{
		X::Value retValue;
		std::vector<X::Value> passInParams;
		X::Hosting::I().SimpleRun("inline_code", jsonStr.c_str(),
			(int)jsonStr.size(),retValue);
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
		std::vector<X::Value> passInParams;
		X::Hosting::I().Run(fileName.c_str(), jsonStr.c_str(),
			(int)jsonStr.size(), passInParams,retValue);
		return retValue;
	}
}