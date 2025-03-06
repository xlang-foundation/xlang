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

#pragma once
#include "xpackage.h"
#include "xlang.h"
#include "exp.h"

namespace X
{
	namespace AST
	{
		class AstNode
		{
			Expression* m_pNode = nullptr;
			X::Value getNodevalue(Expression* pNode);
			X::Value getNodeChildren(Expression* pNode);

		public:
			BEGIN_PACKAGE(AstNode)
				APISET().SetAccessor(&AstNode::Access);
				APISET().AddPropL("parent", [](auto* pThis, X::Value v) {},
					[](auto* pThis) {return pThis->get_parent(); });
				APISET().AddPropL("name", [](auto* pThis, X::Value v) {},
					[](auto* pThis) {return pThis->get_name(); });
				APISET().AddPropL("type", [](auto* pThis, X::Value v) {},
					[](auto* pThis) {return pThis->get_type(); });
				APISET().AddPropL("children", [](auto* pThis, X::Value v) {},
					[](auto* pThis) {return pThis->get_children(); });
				APISET().AddPropL("lineStart", [](auto* pThis, X::Value v) {},
					[](auto* pThis) {return pThis->m_pNode->GetStartLine(); });
				APISET().AddPropL("lineEnd", [](auto* pThis, X::Value v) {},
					[](auto* pThis) {return pThis->m_pNode->GetEndLine(); });
				APISET().AddPropL("CharStart", [](auto* pThis, X::Value v) {},
					[](auto* pThis) {return pThis->m_pNode->GetCharStart(); });
				APISET().AddPropL("CharEnd", [](auto* pThis, X::Value v) {},
					[](auto* pThis) {return pThis->m_pNode->GetCharEnd(); });
				APISET().AddPropL("OffsetInLine", [](auto* pThis, X::Value v) {},
					[](auto* pThis) {return pThis->m_pNode->GetCharPos(); });
			END_PACKAGE
		public:
			AstNode()
			{

			}
			AstNode(Expression* node)
			{
				m_pNode = node;
			}
			void SetNode(Expression* node)
			{
				m_pNode = node;
			}
			X::Value Access(X::Port::vector<X::Value>& IdxAry);
			X::Value get_parent();
			X::Value get_name();
			X::Value get_value();
			X::Value get_type();
			X::Value get_children()
			{
				return getNodeChildren(m_pNode);
			}
		};
		class AstWrapper
		{
			X::Value LoadFromStringImpl(std::string& moduleName,std::string& xcode);
		public:
			BEGIN_PACKAGE(AstWrapper)
				APISET().AddClass<0, AstNode>("Node");
				APISET().AddFunc<1>("loads", &AstWrapper::LoadFromString);
				APISET().AddFunc<1>("load", &AstWrapper::LoadFromFile);
			END_PACKAGE
		public:
			AstWrapper()
			{
			}
			~AstWrapper();
			X::Value LoadFromString(std::string xcode);
			X::Value LoadFromFile(std::string fileName);
		};
	}
}