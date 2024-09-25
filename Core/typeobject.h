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

#include "object.h"

namespace X
{
	namespace Data
	{
		class TypeObject :
			virtual public Object
		{
			std::string m_desc;
			ValueType m_valType = ValueType::Invalid;
			ObjType m_objType = ObjType::Base;

			X::Value m_realObj;

			virtual void GetBaseScopes(std::vector<AST::Scope*>& bases) override;
		public:
			static void Init();
			static void cleanup();

			TypeObject() :Object()
			{
				m_t = ObjType::Type;
			}
			TypeObject(Value& val) :TypeObject()
			{
				m_valType = val.GetType();
				if (m_valType == ValueType::Object)
				{
					m_objType = val.GetObj()->GetType();
				}
				m_realObj = val;
			}
			//help function for type object
			bool IsType(std::string name) 
			{ 
				auto typeName = GetTypeNameString();
				if (typeName == name)
				{
					return true;
				}
				else if (typeName.find("'" + name + "'") > 0)
				{
					return true;
				}
				return false; 
			}
			FORCE_INLINE std::string GetTypeNameString()
			{
				std::string strType;
				switch (m_valType)
				{
				case X::ValueType::Invalid:
					strType = "<class 'Invalid'>";
					break;
				case X::ValueType::None:
					strType = "<class 'NoneType'>";
					break;
				case X::ValueType::Int64:
					strType = "<class 'int'>";
					break;
				case X::ValueType::Double:
					strType = "<class 'float'>";
					break;
				case X::ValueType::Object:
				{
					switch (m_objType)
					{
					case X::ObjType::Base:
						strType = "<class 'object'>";
						break;
					case X::ObjType::Type:
						strType = "<class 'type'>";
						break;
					case X::ObjType::Str:
						strType = "<class 'str'>";
						break;
					case X::ObjType::Binary:
						strType = "<class 'bytes'>";
						break;
					case X::ObjType::Expr:
						strType = "<class 'expression'>";
						break;
					case X::ObjType::ConstExpr:
						strType = "<class 'const expression'>";
						break;
					case X::ObjType::Function:
						strType = "<class 'function'>";
						break;
					case X::ObjType::MetaFunction:
						strType = "<class 'metafunction'>";
						break;
					case X::ObjType::XClassObject:
						strType = "<class 'class'>";
						break;
					case X::ObjType::Prop:
						strType = "<class 'prop'>";
						break;
					case X::ObjType::ObjectEvent:
						strType = "<class 'event'>";
						break;
					case X::ObjType::FuncCalls:
						strType = "<class 'calls'>";
						break;
					case X::ObjType::Package:
						strType = "<class 'package'>";
						break;
					case X::ObjType::ModuleObject:
						strType = "<class 'module'>";
						break;
					case X::ObjType::Future:
						strType = "<class 'future'>";
						break;
					case X::ObjType::Iterator:
						strType = "<class 'iterator'>";
						break;
					case X::ObjType::List:
						strType = "<class 'list'>";
						break;
					case X::ObjType::Dict:
						strType = "<class 'dict'>";
						break;
					case X::ObjType::Set:
						strType = "<class 'set'>";
						break;
					case X::ObjType::Complex:
						strType = "<class 'complex'>";
						break;
					case X::ObjType::TableRow:
						strType = "<class 'tablerow'>";
						break;
					case X::ObjType::Table:
						strType = "<class 'table'>";
						break;
					case X::ObjType::RemoteObject:
						strType = "<class 'remoteobject'>";
						break;
					case X::ObjType::RemoteClientObject:
						strType = "<class 'remoteclientobject'>";
						break;
					case X::ObjType::PyProxyObject:
						strType = "<class 'pythonobject'>";
						break;
					default:
						break;
					}
				}
				break;
				case X::ValueType::Str:
					strType = "<class 'const str'>";
					break;
				case X::ValueType::Value:
					break;
				default:
					break;
				}
				return strType;
			}
			virtual const char* ToString(bool WithFormat = false) override
			{
				auto strRet = GetTypeNameString();
				return GetABIString(strRet);
			}

			X::Value GetMembers(X::XlangRuntime* rt);
		};
	}
}