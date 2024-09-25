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

#include "yaml.h"
#include "yaml_parser.h"
#include "utility.h"
#include "Hosting.h"
#include "port.h"
#include "list.h"
#include "number.h"

namespace X
{
	YamlWrapper::~YamlWrapper()
	{
		if (m_parser)
		{
			delete m_parser;
		}
	}
	X::Value YamlWrapper::LoadFromString(std::string yamlStr)
	{
		if (m_parser)
		{
			delete m_parser;
			m_parser = nullptr;
		}
		Text::YamlParser* ymlP = new Text::YamlParser();
		ymlP->Init();
		ymlP->LoadFromString((char*)yamlStr.c_str(), (int)yamlStr.size());
		bool bOK = ymlP->Parse();
		if (!bOK)
		{
			return X::Value(false);
		}
		m_parser = ymlP;
		auto* rootNode = ymlP->GetRoot();
		X::XPackageValue<YamlNodeWrapper> valNode;
		(*valNode).SetNode(rootNode);
		return valNode;
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
		std::string yamlStr;
		bool bOK = LoadStringFromFile(fileName, yamlStr);
		if (!bOK)
		{
			return X::Value(false);
		}
		return LoadFromString(yamlStr);
	}

	static X::Value ConvertToValue(std::string& strValue)
	{
		double dVal = 0;
		long long llVal = 0;
		X::String xs{ (char*)strValue.c_str(),(int)strValue.size() };
		auto state = ParseNumber(xs, dVal, llVal);
		X::Value retVal;
		if (state == X::ParseState::Long_Long)
		{
			retVal = X::Value(llVal);
		}
		else if (state == X::ParseState::Double)
		{
			retVal = X::Value(dVal);
		}
		else
		{
			retVal = X::Value(strValue);
		}
		return retVal;
	}
	X::Value YamlNodeWrapper::getNodevalue(Text::YamlNode* pNode)
	{
		if (pNode)
		{
			if (pNode->IsSingleValueType())
			{
				std::string strValue = pNode->GetValue();
				if (pNode->HaveQuote())
				{
					return X::Value(strValue);
				}
				else
				{
					return ConvertToValue(strValue);
				}
			}
			else
			{
				auto* pValueNode = pNode->GetValueNode();
				if (pValueNode)
				{
					if (pValueNode->IsSingleValueType())
					{
						std::string strValue = pValueNode->GetValue();
						if (pValueNode->HaveQuote())
						{
							return X::Value(strValue);
						}
						else
						{
							return ConvertToValue(strValue);
						}

					}
					else
					{
						X::XPackageValue<YamlNodeWrapper> valNode;
						(*valNode).SetNode(pValueNode);
						return valNode;
					}
				}
				else
				{
					return getNodeChildren(pNode);
				}
			}
		}
		else
		{
			return X::Value();
		}
	}
	X::Value YamlNodeWrapper::Access(X::Port::vector<X::Value>& IdxAry)
	{
		Text::YamlNode* pCurNode = m_pNode;
		Text::YamlNode* pFindNode = nullptr;
		bool bFind = true;
		for (auto& idx : IdxAry)
		{
			if (pCurNode == nullptr)
			{
				bFind = false;
				break;
			}
			if (idx.IsLong())
			{
				int nIdx = (int)idx.GetLongLong();
				int itemCount = pCurNode->GetChildrenCount();
				if (itemCount > nIdx)
				{
					pCurNode = pCurNode->GetChild(nIdx);
				}
				else
				{
					auto* pValueNode = pCurNode->GetValueNode();
					int itemCount = pValueNode->GetChildrenCount();
					if (itemCount > nIdx)
					{
						pCurNode = pValueNode->GetChild(nIdx);
					}
					else
					{//error
						pCurNode = nullptr;
						bFind = false;
						break;
					}
				}
			}
			else
			{
				std::string strIdx = idx.ToString();
				auto* pNode0 = pCurNode->FindNode(strIdx);
				if (pNode0 == nullptr)
				{
					auto* pValueNode = pCurNode->GetValueNode();
					if (pValueNode)
					{
						pNode0 = pValueNode->FindNode(strIdx);
					}
				}
				pCurNode = pNode0;
			}
		}
		if (bFind)
		{
			if (pCurNode)
			{
				return getNodevalue(pCurNode);
			}
			else
			{
				return X::Value();
			}
		}
		return X::Value();
	}
	X::Value YamlNodeWrapper::get_parent()
	{
		if (m_pNode)
		{
			X::XPackageValue<YamlNodeWrapper> valNode;
			(*valNode).SetNode(m_pNode->Parent());
			return valNode;
		}
		else
		{
			return X::Value();
		}
	}

	X::Value YamlNodeWrapper::get_key()
	{
		if (m_pNode)
		{
			if (m_pNode->IsSingleValueType())
			{
				return X::Value();
			}
			else
			{
				std::string strValue = m_pNode->GetValue();
				return X::Value(strValue);
			}
		}
		else
		{
			return X::Value();
		}
	}

	X::Value YamlNodeWrapper::get_value()
	{
		return getNodevalue(m_pNode);
	}

	X::Value YamlNodeWrapper::get_size()
	{
		if (m_pNode)
		{
			return X::Value(m_pNode->GetChildrenCount());
		}
		else
		{
			return X::Value(0);
		}
	}
	X::Value YamlNodeWrapper::getNodeChildren(Text::YamlNode* pNode)
	{
		if (pNode == nullptr)
		{
			return X::Value();
		}
		X::List list;
		auto& nodes = pNode->GetChildren();
		for (auto* node : nodes)
		{
			X::XPackageValue<YamlNodeWrapper> valNode;
			(*valNode).SetNode(node);
			list += valNode;
		}
		return list;
	}
	X::Value YamlNodeWrapper::get_children()
	{
		return getNodeChildren(m_pNode);
	}

}