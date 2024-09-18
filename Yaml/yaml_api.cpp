#include "yaml_api.h"
#include "port.h"
#include <fstream>

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

	std::string ToAbsFilePath(X::XRuntime* rt,std::string& fileName)
	{
		if (!IsAbsPath(fileName))
		{
			std::string curPath = rt->GetXModuleFileName();
			//Strip the file name
			auto pos = curPath.rfind('\\');
			if (pos != std::string::npos)
			{
				curPath = curPath.substr(0, pos);
			}
			else
			{
				pos = curPath.rfind('/');
				if (pos != std::string::npos)
				{
					curPath = curPath.substr(0, pos);
				}
			}
			fileName = curPath + Path_Sep_S + fileName;
		}
		return fileName;
	}
	bool ReadFileToString(const std::string& fileName,std::string& content) 
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
		// Try parsing as int
		try {
			int value = scalarNode.as<int>();
			return X::Value(value);  // XValue auto converts int
		}
		catch (const YAML::BadConversion&) {}

		// Try parsing as float
		try {
			float value = scalarNode.as<float>();
			return X::Value(value);  // XValue auto converts float
		}
		catch (const YAML::BadConversion&) {}

		// Try parsing as double
		try {
			double value = scalarNode.as<double>();
			return X::Value(value);  // XValue auto converts double
		}
		catch (const YAML::BadConversion&) {}

		// Try parsing as unsigned long long
		try {
			unsigned long long value = scalarNode.as<unsigned long long>();
			return X::Value(value);  // XValue auto converts unsigned long long
		}
		catch (const YAML::BadConversion&) {}

		// Try parsing as bool
		try {
			bool value = scalarNode.as<bool>();
			return X::Value(value);  // XValue auto converts bool
		}
		catch (const YAML::BadConversion&) {}

		// If all type conversions fail, fallback to string
		std::string value = scalarNode.as<std::string>();
		return X::Value(value);
	}

	X::Value TranslateYamlToXValue(const YAML::Node& rootNode) 
	{
		if (rootNode.IsScalar()) {
			// Handle scalar values
			return ConvertYamlScalarToXValue(rootNode);
		}
		else if (rootNode.IsSequence()) {
			// Handle sequence (YAML array)
			X::List list;
			for (const auto& item : rootNode) {
				X::Value value = TranslateYamlToXValue(item);
				list += value;
			}
			return X::Value(list);  // Convert XList to XValue
		}
		else if (rootNode.IsMap()) {
			// Handle mapping (YAML dictionary)
			X::Dict dict;
			for (const auto& item : rootNode) {
				X::Value key = item.first.as<std::string>();
				X::Value value = TranslateYamlToXValue(item.second);
				dict->Set(key, value);
			}
			return X::Value(dict); 
		}
		else if (rootNode.IsNull()) {
			// Handle explicit null values in YAML
			return X::Value(X::ValueType::None);
		}

		return X::Value();
	}

	YAML::Node ConvertXValueToYamlNode(X::Value value)
	{
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
				yamlNode.push_back(ConvertXValueToYamlNode(item));
			}
		}
		else if (value.IsDict()) {
			X::Dict dict(value);
			dict->Enum([&yamlNode](X::Value& varKey, X::Value& val) {
					std::string key = varKey.ToString();
					YAML::Node keyNode = YAML::Node(key);
					yamlNode[keyNode] = ConvertXValueToYamlNode(val);
				});
		}
		else {
			yamlNode = YAML::Node(YAML::NodeType::Null);
		}

		return yamlNode;
	}

	// Function to save X::Value to a YAML string
	std::string Yaml_Wrapper::SaveToYamlString(const X::Value& value)
	{
		YAML::Emitter out;
		YAML::Node rootNode = ConvertXValueToYamlNode(value);
		out << rootNode;
		return std::string(out.c_str());
	}

	// Function to save X::Value to a YAML file
	bool Yaml_Wrapper::SaveToYamlFile(X::XRuntime* rt, X::XObj* pContext, 
		X::Value& value,std::string fileName)
	{
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


	X::Value Yaml_Wrapper::LoadFromString(std::string yamlStr)
	{
		YAML::Node rootNode = YAML::Load(yamlStr);
		X::Value varNode = TranslateYamlToXValue(rootNode);
		return varNode;
	}
	X::Value  Yaml_Wrapper::LoadFromFile(X::XRuntime* rt, X::XObj* pContext,
		std::string fileName)
	{
		std::string fileNameAbs = ToAbsFilePath(rt, fileName);
		std::string yamlStr;
		bool bOK = ReadFileToString(fileNameAbs, yamlStr);
		if (!bOK) {
			std::string msg = "No file:" + fileNameAbs;
			return X::Error(-100, msg.c_str());
		}
		YAML::Node rootNode = YAML::Load(yamlStr);
		X::Value varNode = TranslateYamlToXValue(rootNode);
		return varNode;
	}
}