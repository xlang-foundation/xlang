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
#include "var.h"
#include "runtime.h"

namespace X
{
	namespace Data
	{
		class Iterator :
			virtual public XIterator,
			virtual public Object
		{
			AST::Expression* m_varToImpack = nullptr;
			X::Value m_container;
			Iterator_Pos m_pos = nullptr;
		public:
			Iterator():
				XIterator(0)
			{
				m_t = ObjType::Iterator;
			}
			FORCE_INLINE void SetImpactVar(AST::Expression* pVar)
			{
				m_varToImpack = pVar;
			}
			FORCE_INLINE void SetContainer(X::Value& v)
			{
				m_container = v;
			}
			FORCE_INLINE ~Iterator()
			{
				if (m_container.IsObject())
				{
					auto* pDataObj = dynamic_cast<Data::Object*>(m_container.GetObj());
					pDataObj->CloseIterator(m_pos);
				}
			}
			virtual bool Call(XRuntime* rt, XObj* pContext,
				ARGS& params,
				KWARGS& kwParams,
				X::Value& retValue) override
			{
				if (m_container.IsObject())
				{
					auto* pDataObj = dynamic_cast<Data::Object*>(m_container.GetObj());
					std::vector<Value> vals;
					if (pDataObj->GetAndUpdatePos(m_pos, vals,false))
					{
						m_varToImpack->SetArry(dynamic_cast<XlangRuntime*>(rt), 
							pContext, vals);
						retValue = X::Value(true);
					}
					else
					{
						retValue = X::Value(false);
					}
				}
				else
				{
					retValue = X::Value(false);
				}
				return true;
			}
		};
	}
}
