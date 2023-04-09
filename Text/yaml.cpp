#include "yaml.h"
#include "yaml_parser.h"
#include "utility.h"
#include "Hosting.h"
#include "port.h"

namespace X
{
	X::Value YamlWrapper::LoadFromString(std::string yamlStr)
	{
		X::Value retValue;
		Text::YamlParser yml;
		yml.Init();
		yml.LoadFromString((char*)yamlStr.c_str(), (int)yamlStr.size());
		yml.Parse();
		return retValue;
	}
	X::Value  YamlWrapper::LoadFromFile(X::XRuntime* rt, X::XObj* pContext,
		std::string fileName)
	{
		if (!IsAbsPath(fileName))
		{
			X::XlangRuntime* pRt = (X::XlangRuntime*)rt;
			std::string curPath = pRt->M()->GetModulePath();
			fileName = curPath + Path_Sep_S + fileName;
		}
		X::Value retValue;
		std::string yamlStr;
		bool bOK = LoadStringFromFile(fileName, yamlStr);
		if (!bOK)
		{
			retValue = X::Value(false);
		}
		Text::YamlParser yml;
		yml.Init();
		yml.LoadFromString((char*)yamlStr.c_str(), (int)yamlStr.size());
		yml.Parse();

		return retValue;
	}
}