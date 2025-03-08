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

#include "yaml_api.h"
#include "port.h"
#include <fstream>
#include <filesystem>
#include <string>
#include <regex>

#include <yaml-cpp/yaml.h>


namespace X
{
	bool IsAbsPath(std::string& strPath)
	{
		bool bIsAbs = false;
#if (WIN32)
		if (strPath.find(":/") != std::string::npos
			|| strPath.find(":\\") != std::string::npos
			|| strPath.find("\\\\") != std::string::npos//network path
			|| strPath.find("//") != std::string::npos//network path
			)
		{
			bIsAbs = true;
		}
#else
		if (strPath.find('/') == 0)
		{
			bIsAbs = true;
		}
#endif
		return bIsAbs;
	}
	std::string ToAbsFilePath(X::XRuntime* rt, std::string& fileName)
	{
		namespace fs = std::filesystem;

		// Check if the path is already absolute
		if (!fs::path(fileName).is_absolute())
		{
			// Get the current module's directory path
			std::string xFileName = rt->GetXModuleFileName();
			fs::path curPath = xFileName;
			curPath = curPath.parent_path(); // Get the parent directory

			// Combine the current path with the provided file name
			fileName = (curPath / fileName).string();
		}

		// Normalize the path to use the correct separators for the current platform
		return fs::absolute(fileName).string();
	}

	bool ReadFileToString(const std::string& fileName, std::string& content)
	{
		std::ifstream file(fileName);
		if (!file.is_open())
		{
			return false;
		}
		content = std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		return true;
	}

	X::Value ConvertYamlScalarToXValue(const YAML::Node& scalarNode)
	{
		// Defensively check the node type to avoid exceptions
		if (!scalarNode.IsScalar()) {
			return X::Value(std::string(""));
		}

		std::string rawStr = scalarNode.Scalar();

		// Check for integer format: optional sign followed by digits only
		if (std::regex_match(rawStr, std::regex("^[-+]?[0-9]+$"))) {
			try {
				int value = std::stoi(rawStr);
				return X::Value(value);
			}
			catch (...) {
				try {
					long long value = std::stoll(rawStr);
					return X::Value(value);
				}
				catch (...) {}
			}
		}

		// Check for floating point format
		if (std::regex_match(rawStr, std::regex("^[-+]?[0-9]*\\.?[0-9]+([eE][-+]?[0-9]+)?$"))) {
			try {
				double value = std::stod(rawStr);
				return X::Value(value);
			}
			catch (...) {}
		}

		// Check for boolean values
		if (rawStr == "true" || rawStr == "True" || rawStr == "TRUE" ||
			rawStr == "yes" || rawStr == "Yes" || rawStr == "YES" ||
			rawStr == "on" || rawStr == "On" || rawStr == "ON") {
			return X::Value(true);
		}

		if (rawStr == "false" || rawStr == "False" || rawStr == "FALSE" ||
			rawStr == "no" || rawStr == "No" || rawStr == "NO" ||
			rawStr == "off" || rawStr == "Off" || rawStr == "OFF") {
			return X::Value(false);
		}

		// If no other type matches, return as string
		return X::Value(rawStr);
	}

	X::Value TranslateYamlToXValue(const YAML::Node& rootNode)
	{
		// Extra defensive check for null node
		if (!rootNode.IsDefined() || rootNode.IsNull()) {
			return X::Value(X::ValueType::None);
		}

		try {
			if (rootNode.IsScalar()) {
				// Handle scalar values 
				return ConvertYamlScalarToXValue(rootNode);
			}
			else if (rootNode.IsSequence()) {
				// Handle sequence (YAML array)
				X::List list;
				for (const auto& item : rootNode) {
					// Skip over invalid nodes
					if (!item.IsDefined()) {
						continue;
					}
					X::Value value = TranslateYamlToXValue(item);
					list += value;
				}
				return X::Value(list);
			}
			else if (rootNode.IsMap()) {
				// Handle mapping (YAML dictionary)
				X::Dict dict;
				for (const auto& item : rootNode) {
					// Skip invalid keys
					if (!item.first.IsDefined() || !item.first.IsScalar()) {
						continue;
					}

					// Skip invalid values
					if (!item.second.IsDefined()) {
						continue;
					}

					try {
						std::string keyStr = item.first.Scalar();  // Use Scalar() instead of as<string>()
						X::Value key = keyStr;
						X::Value value = TranslateYamlToXValue(item.second);
						dict->Set(key, value);
					}
					catch (...) {
						// Skip this item if any error occurs
						continue;
					}
				}
				return X::Value(dict);
			}
		}
		catch (...) {
			// Return empty value on any unexpected exception
			return X::Value();
		}

		// Default fallback
		return X::Value();
	}

	YAML::Node ConvertXValueToYamlNode(X::Value value)
	{
		try {
			YAML::Node yamlNode;

			if (value.IsLong()) {
				yamlNode = (long)value;
			}
			else if (value.IsDouble()) {
				yamlNode = (double)value;
			}
			else if (value.IsBool()) {
				yamlNode = (bool)value;
			}
			else if (value.IsString()) {
				yamlNode = value.ToString();
			}
			else if (value.IsList()) {
				X::List list(value);
				for (const auto& item : *list) {
					try {
						yamlNode.push_back(ConvertXValueToYamlNode(item));
					}
					catch (...) {
						// If conversion fails for an item, add null
						yamlNode.push_back(YAML::Node(YAML::NodeType::Null));
					}
				}
			}
			else if (value.IsDict()) {
				X::Dict dict(value);
				dict->Enum([&yamlNode](X::Value& varKey, X::Value& val) {
					try {
						std::string key = varKey.ToString();
						YAML::Node keyNode = YAML::Node(key);
						yamlNode[keyNode] = ConvertXValueToYamlNode(val);
					}
					catch (...) {
						// Skip this entry if conversion fails
					}
					});
			}
			else {
				yamlNode = YAML::Node(YAML::NodeType::Null);
			}

			return yamlNode;
		}
		catch (...) {
			// Return empty node on failure
			return YAML::Node(YAML::NodeType::Null);
		}
	}

	// Function to save X::Value to a YAML string
	std::string Yaml_Wrapper::SaveToYamlString(const X::Value& value)
	{
		try {
			YAML::Emitter out;
			YAML::Node rootNode = ConvertXValueToYamlNode(value);
			out << rootNode;
			return std::string(out.c_str());
		}
		catch (const YAML::Exception& e) {
			// Return error message as string on failure
			return "YAML serialization error: " + std::string(e.what());
		}
		catch (const std::exception& e) {
			return "Error saving to YAML: " + std::string(e.what());
		}
		catch (...) {
			return "Unknown error saving to YAML string";
		}
	}

	// Function to save X::Value to a YAML file
	bool Yaml_Wrapper::SaveToYamlFile(X::XRuntime* rt, X::XObj* pContext,
		X::Value& value, std::string fileName)
	{
		try {
			std::string fileNameAbs = ToAbsFilePath(rt, fileName);
			std::ofstream outFile(fileNameAbs);
			if (!outFile.is_open()) {
				return false;
			}
			std::string yamlStr = SaveToYamlString(value);
			outFile << yamlStr;
			outFile.close();
			return true;
		}
		catch (const std::exception&) {
			return false;
		}
		catch (...) {
			return false;
		}
	}

	X::Value Yaml_Wrapper::LoadFromString(std::string yamlStr)
	{
		try {
			YAML::Node rootNode = YAML::Load(yamlStr);
			X::Value varNode = TranslateYamlToXValue(rootNode);
			return varNode;
		}
		catch (const YAML::ParserException& e) {
			std::string msg = "YAML parsing error: " + std::string(e.what());
			return X::Error(-101, msg.c_str());
		}
		catch (const std::exception& e) {
			std::string msg = "Error loading YAML: " + std::string(e.what());
			return X::Error(-102, msg.c_str());
		}
		catch (...) {
			return X::Error(-103, "Unknown error loading YAML string");
		}
	}

	X::Value Yaml_Wrapper::LoadFromFile(X::XRuntime* rt, X::XObj* pContext,
		std::string fileName)
	{
		try {
			std::string fileNameAbs = ToAbsFilePath(rt, fileName);
			std::string yamlStr;
			bool bOK = ReadFileToString(fileNameAbs, yamlStr);
			if (!bOK) {
				std::string msg = "No file:" + fileNameAbs;
				return X::Error(-100, msg.c_str());
			}

			return LoadFromString(yamlStr);
		}
		catch (const std::exception& e) {
			std::string msg = "Error loading YAML file: " + std::string(e.what());
			return X::Error(-104, msg.c_str());
		}
		catch (...) {
			return X::Error(-105, "Unknown error loading YAML file");
		}
	}
}