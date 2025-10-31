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
#include "utility.h"
#include "port.h"
#include "dict.h"
#include "list.h"
#include <string>
#include <fstream>
#include <stdexcept>

namespace X
{
	// ============================================================================
	// Helper: Normalize JSON string (handle escape sequences)
	// ============================================================================
	std::string JsonWrapper::NormalizeJsonString(const std::string& jsonStr)
	{
		std::string result = jsonStr;

		// Strip surrounding double quotes if present
		if (!result.empty() && result.front() == '"' && result.back() == '"')
		{
			result = result.substr(1, result.size() - 2);
		}

		// Replace escaped newlines with actual newlines
		size_t pos = 0;
		while ((pos = result.find("\\n", pos)) != std::string::npos)
		{
			result.replace(pos, 2, "\n");
			pos += 1; // Move past the replaced '\n'
		}

		// Replace escaped tabs with actual tabs
		pos = 0;
		while ((pos = result.find("\\t", pos)) != std::string::npos)
		{
			result.replace(pos, 2, "\t");
			pos += 1;
		}

		// Replace escaped quotes with actual quotes
		pos = 0;
		while ((pos = result.find("\\\"", pos)) != std::string::npos)
		{
			result.replace(pos, 2, "\"");
			pos += 1;
		}

		// Replace escaped backslashes
		pos = 0;
		while ((pos = result.find("\\\\", pos)) != std::string::npos)
		{
			result.replace(pos, 2, "\\");
			pos += 1;
		}

		return result;
	}

	// ============================================================================
	// Convert nlohmann::json to X::Value (similar to YAML's getNodevalue)
	// ============================================================================
	X::Value JsonWrapper::ConvertJsonToXValue(const nlohmann::json& j)
	{
		try
		{
			if (j.is_null())
			{
				return X::Value();
			}
			else if (j.is_boolean())
			{
				return X::Value(j.get<bool>());
			}
			else if (j.is_number_integer())
			{
				// Handle both signed and unsigned integers
				if (j.is_number_unsigned())
				{
					return X::Value(static_cast<long long>(j.get<uint64_t>()));
				}
				else
				{
					return X::Value(j.get<long long>());
				}
			}
			else if (j.is_number_float())
			{
				return X::Value(j.get<double>());
			}
			else if (j.is_string())
			{
				return X::Value(j.get<std::string>());
			}
			else if (j.is_array())
			{
				X::List list;
				for (const auto& element : j)
				{
					list += ConvertJsonToXValue(element);
				}
				return list;
			}
			else if (j.is_object())
			{
				X::Dict dict;
				for (auto it = j.begin(); it != j.end(); ++it)
				{
					dict->Set(it.key(), ConvertJsonToXValue(it.value()));
				}
				return dict;
			}
			else
			{
				// Unsupported type, return null
				return X::Value();
			}
		}
		catch (const std::exception& e)
		{
			// Log error if logging is available, for now return null value
			return X::Value();
		}
	}

	// ============================================================================
	// Convert X::Value to nlohmann::json (inverse of above)
	// ============================================================================
	nlohmann::json JsonWrapper::ConvertXValueToJson(X::Value& value)
	{
		try
		{
			if (value.IsLong())
			{
				return nlohmann::json(value.GetLongLong());
			}
			else if (value.IsDouble())
			{
				return nlohmann::json(static_cast<double>(value));
			}
			else if (value.IsBool())
			{
				return nlohmann::json(static_cast<bool>(value));
			}
			else if (value.IsString())
			{
				return nlohmann::json(value.ToString());
			}
			else if (value.IsList())
			{
				nlohmann::json jsonArray = nlohmann::json::array();
				X::List list(value);
				for (auto item : *list)
				{
					jsonArray.push_back(ConvertXValueToJson(item));
				}
				return jsonArray;
			}
			else if (value.IsDict())
			{
				nlohmann::json jsonObject = nlohmann::json::object();
				X::Dict dict(value);
				dict->Enum([&jsonObject, this](X::Value& varKey, X::Value& val)
					{
						std::string key = varKey.ToString();
						jsonObject[key] = ConvertXValueToJson(val);
					});
				return jsonObject;
			}
			else
			{
				// Unsupported type or null
				return nlohmann::json(nullptr);
			}
		}
		catch (const std::exception& e)
		{
			// Return null on error
			return nlohmann::json(nullptr);
		}
	}

	// ============================================================================
	// LoadFromString: Parse JSON string and convert to X::Value
	// ============================================================================
	X::Value JsonWrapper::LoadFromString(X::XRuntime* rt, X::XObj* pContext,
		X::ARGS& params, X::KWARGS& kwParams, X::Value& retValue)
	{
		if (params.size() < 1)
		{
			retValue = X::Value(false);
			return retValue;
		}

		std::string jsonStr = params[0].ToString();

		// Check for normalize parameter
		bool needNormalize = false;
		auto it = kwParams.find("normalize");
		if (it)
		{
			needNormalize = static_cast<bool>(it->val);
		}

		if (needNormalize)
		{
			jsonStr = NormalizeJsonString(jsonStr);
		}

		// Parse JSON using nlohmann::json
		try
		{
			nlohmann::json j = nlohmann::json::parse(jsonStr);
			retValue = ConvertJsonToXValue(j);
			return retValue;
		}
		catch (const nlohmann::json::parse_error& e)
		{
			// Parse error - return false to indicate failure
			retValue = X::Value(false);
			return retValue;
		}
		catch (const std::exception& e)
		{
			// Other errors
			retValue = X::Value(false);
			return retValue;
		}
	}

	// ============================================================================
	// LoadFromFile: Load JSON from file and convert to X::Value
	// ============================================================================
	X::Value JsonWrapper::LoadFromFile(X::XRuntime* rt, X::XObj* pContext,
		std::string fileName)
	{
		// Resolve relative paths
		if (!IsAbsPath(fileName))
		{
			X::XlangRuntime* pRt = (X::XlangRuntime*)rt;
			std::string curPath = pRt->M()->GetModulePath();
			fileName = curPath + Path_Sep_S + fileName;
		}

		// Load file contents
		std::string jsonStr;
		bool bOK = LoadStringFromFile(fileName, jsonStr);
		if (!bOK)
		{
			return X::Value(false);
		}

		// Parse JSON using nlohmann::json
		try
		{
			nlohmann::json j = nlohmann::json::parse(jsonStr);
			X::Value retValue = ConvertJsonToXValue(j);
			return retValue;
		}
		catch (const nlohmann::json::parse_error& e)
		{
			// Parse error
			return X::Value(false);
		}
		catch (const std::exception& e)
		{
			// Other errors
			return X::Value(false);
		}
	}

	// ============================================================================
	// SaveToString: Convert X::Value to JSON string
	// ============================================================================
	bool JsonWrapper::SaveToString(X::XRuntime* rt, X::XObj* pContext,
		X::ARGS& params, X::KWARGS& kwParams, X::Value& retValue)
	{
		if (params.size() < 1)
		{
			retValue = X::Value("");
			return false;
		}

		// Get pretty print option
		bool prettyPrint = false;
		if (params.size() > 1)
		{
			prettyPrint = params[1].ToBool();
		}
		else
		{
			auto it = kwParams.find("pretty");
			if (it)
			{
				prettyPrint = static_cast<bool>(it->val);
			}
		}

		// Get indent size (default 4 spaces)
		int indent = 4;
		auto indentIt = kwParams.find("indent");
		if (indentIt)
		{
			indent = static_cast<int>(indentIt->val.GetLongLong());
			if (indent < 0) indent = 4; // Sanity check
		}

		// Convert X::Value to JSON
		try
		{
			X::Value value = params[0];
			nlohmann::json j = ConvertXValueToJson(value);

			// Serialize to string
			std::string jsonStr;
			if (prettyPrint)
			{
				jsonStr = j.dump(indent);
			}
			else
			{
				jsonStr = j.dump();
			}

			retValue = X::Value(jsonStr);
			return true;
		}
		catch (const std::exception& e)
		{
			retValue = X::Value("");
			return false;
		}
	}

	// ============================================================================
	// SaveToFile: Convert X::Value to JSON and save to file
	// ============================================================================
	bool JsonWrapper::SaveToFile(X::XRuntime* rt, X::XObj* pContext,
		X::ARGS& params, X::KWARGS& kwParams, X::Value& retValue)
	{
		if (params.size() < 2)
		{
			retValue = X::Value(false);
			return false;
		}

		// Get pretty print option
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
				prettyPrint = static_cast<bool>(it->val);
			}
		}

		// Get indent size (default 4 spaces)
		int indent = 4;
		auto indentIt = kwParams.find("indent");
		if (indentIt)
		{
			indent = static_cast<int>(indentIt->val.GetLongLong());
			if (indent < 0) indent = 4; // Sanity check
		}

		// Convert X::Value to JSON
		try
		{
			X::Value value = params[0];
			nlohmann::json j = ConvertXValueToJson(value);

			// Serialize to string
			std::string jsonStr;
			if (prettyPrint)
			{
				jsonStr = j.dump(indent);
			}
			else
			{
				jsonStr = j.dump();
			}

			// Resolve file path
			std::string fileName = params[1].ToString();
			if (!IsAbsPath(fileName))
			{
				X::XlangRuntime* pRt = (X::XlangRuntime*)rt;
				std::string curPath = pRt->M()->GetModulePath();
				fileName = curPath + Path_Sep_S + fileName;
			}

			// Write to file
			std::ofstream file(fileName);
			if (!file.is_open())
			{
				retValue = X::Value(false);
				return false;
			}

			file << jsonStr;
			file.close();

			retValue = X::Value(true);
			return true;
		}
		catch (const std::exception& e)
		{
			retValue = X::Value(false);
			return false;
		}
	}

} // namespace X