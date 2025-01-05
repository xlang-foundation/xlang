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
#include <string>
#include <sstream>
#include <fstream>


//TODO: for security, disable any function call inside json string or file

namespace X
{
    X::Value JsonWrapper::LoadFromString(X::XRuntime* rt, X::XObj* pContext,
        X::ARGS& params, X::KWARGS& kwParams, X::Value& retValue)
    {
        if (params.size() < 1)
        {
            return X::Value();
        }

        std::string jsonStr = params[0].ToString();

        bool needNormalize = false;
        auto it = kwParams.find("normalize");
        if (it)
        {
            needNormalize = (bool)it->val;
        }

        if (needNormalize)
        {
            // Strip surrounding double quotes
            if (!jsonStr.empty() && jsonStr.front() == '"' && jsonStr.back() == '"')
            {
                jsonStr = jsonStr.substr(1, jsonStr.size() - 2);
            }

            // Replace "\n" with '\n'
            size_t pos = 0;
            while ((pos = jsonStr.find("\\n", pos)) != std::string::npos)
            {
                jsonStr.replace(pos, 2, "\n");
                pos += 1; // Move past the replaced '\n'
            }
        }

        std::vector<X::Value> passInParams;
        X::Hosting::I().SimpleRun("inline_code", jsonStr.c_str(),
            (int)jsonStr.size(), retValue);

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

    bool JsonWrapper::SaveToString(X::XRuntime* rt, X::XObj* pContext,
        X::ARGS& params, X::KWARGS& kwParams, X::Value& retValue)
    {
		if (params.size() < 1)
		{
			return "";
		}
        X::Value value;
        bool prettyPrint = false;
		if (params.size() > 1) {
			prettyPrint = params[1].ToBool();
		}
        else {
			auto it = kwParams.find("pretty");
            if (it){
                prettyPrint = (bool)it->val;
            }
        }
		value = params[0];
        retValue = ConvertXValueToJsonString(value, prettyPrint,0);
		return true;
    }

    bool JsonWrapper::SaveToFile(X::XRuntime* rt, 
        X::XObj* pContext, X::ARGS& params, X::KWARGS& kwParams, X::Value& retValue)
    {
        if (params.size() < 2)
        {
			retValue = false;
			return false;
        }
        bool prettyPrint = false;
        if (params.size() > 2) 
        {
            prettyPrint = params[2].ToBool();
        }
        else 
        {
            auto it = kwParams.find("pretty");
            if (it) 
            {
                prettyPrint = (bool)it->val;
            }
        }
		X::Value value = params[0];
		std::string jsonStr = ConvertXValueToJsonString(value, prettyPrint, 0);
		std::string fileName = params[1].ToString();
        if (!IsAbsPath(fileName))
        {
			X::XlangRuntime* pRt = (X::XlangRuntime*)rt;
			std::string curPath = pRt->M()->GetModulePath();
			fileName = curPath + Path_Sep_S + fileName;
        }
        //write to file
        try 
        {
            std::ofstream file(fileName);
            if (file.is_open())
            {
                file << jsonStr;
                file.close();
            }
		}
		catch (const std::exception& e) 
        {
			retValue = false;
			return false;
		}
		retValue = true;
        return true;
    }

    inline std::string GetIndentation(int indentLevel) {
        return std::string(indentLevel * 4, ' '); // 4 spaces per indentation level
    }
    std::string JsonWrapper::ConvertXValueToJsonString(X::Value value, 
        bool prettyPrint, int indentLevel) {
        std::string jsonStr;

        if (value.IsLong()) {
            jsonStr = std::to_string((long)value);
        }
        else if (value.IsDouble()) {
            jsonStr = std::to_string((double)value);
        }
        else if (value.IsBool()) {
            jsonStr = value ? "true" : "false";
        }
        else if (value.IsString()) {
            jsonStr = "\"" + value.ToString() + "\""; // Add quotes for JSON strings
        }
        else if (value.IsList()) {
            jsonStr = "[";
            if (prettyPrint) jsonStr += "\r\n";
            X::List list(value);
            bool first = true;
            for (const auto& item : *list) {
                if (!first) {
                    jsonStr += ",";
                    if (prettyPrint) jsonStr += "\r\n";
                }
                if (prettyPrint) jsonStr += GetIndentation(indentLevel + 1);
                jsonStr += ConvertXValueToJsonString(item, prettyPrint, indentLevel + 1);
                first = false;
            }
            if (prettyPrint) {
                jsonStr += "\r\n" + GetIndentation(indentLevel);
            }
            jsonStr += "]";
        }
        else if (value.IsDict()) {
            jsonStr = "{";
            if (prettyPrint) jsonStr += "\r\n";
            X::Dict dict(value);
            bool first = true;
            dict->Enum([&jsonStr, &first, prettyPrint, indentLevel,this](
                X::Value& varKey, X::Value& val) {
                if (!first) {
                    jsonStr += ",";
                    if (prettyPrint) jsonStr += "\r\n";
                }
                if (prettyPrint) jsonStr += GetIndentation(indentLevel + 1);
                jsonStr += "\"" + varKey.ToString() + "\": " 
                    + ConvertXValueToJsonString(val, prettyPrint, indentLevel + 1);
                first = false;
                });
            if (prettyPrint) {
                jsonStr += "\r\n" + GetIndentation(indentLevel);
            }
            jsonStr += "}";
        }
        else {
            // Handle unsupported types or null values
            jsonStr = "null";
        }

        return jsonStr;
    }


}