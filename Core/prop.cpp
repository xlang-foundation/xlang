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

#include "prop.h"
#include "list.h"
#include "dict.h"

namespace X
{
	namespace Data
	{
		List* PropObject::FlatPack(XlangRuntime* rt, XObj* pContext,
			std::vector<std::string>& IdList, int id_offset,
			long long startIndex, long long count)
		{
			List* pOutList = nullptr;
			X::Value v0;
			if (GetPropValue(rt, pContext, v0))
			{
				if (v0.IsObject())
				{
					Object* pObj = dynamic_cast<Object*>(v0.GetObj());
					pOutList = pObj->FlatPack(rt, nullptr, IdList, id_offset,startIndex, count);
				}
			}
			return pOutList;
		}
		X::Value PropObject::UpdateItemValue(XlangRuntime* rt, XObj* pContext,
			std::vector<std::string>& IdList, int id_offset,
			std::string itemName, X::Value& val)
		{
			X::Value  retVal = val;
			X::Value v0;
			if (GetPropValue(rt, pContext, v0))
			{
				if (v0.IsObject())
				{
					Object* pObj = dynamic_cast<Object*>(v0.GetObj());
					retVal = pObj->UpdateItemValue(rt, nullptr, IdList, id_offset,itemName,val);
				}
			}
			return retVal;
		}
	}
}