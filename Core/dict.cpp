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

#include "dict.h"
#include "list.h"
#include "port.h"
#include "function.h"
#include "obj_func_scope.h"
#include "op.h"
#include "internal_assign.h"

namespace X
{
	namespace Data
	{
		static Obj_Func_Scope<7> _dictScope;
		void Dict::Init()
		{
			_dictScope.Init();
			{
				auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					X::ARGS& params,
					X::KWARGS& kwParams,
					X::Value& retValue)
					{
						if (params.size() == 0)
						{
							return false;
						}
						Dict* pObj = dynamic_cast<Dict*>(pContext);
						retValue = X::Value(pObj->Compare(params[0]));
						return true;
					};
				_dictScope.AddFunc("compare", "compare(another-Dict)", f);
			}
			{
				auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					X::ARGS& params,
					X::KWARGS& kwParams,
					X::Value& retValue)
				{
					Dict* pObj = dynamic_cast<Dict*>(pContext);
					retValue = X::Value(pObj->Has(params[0]));
					return true;
				};
				_dictScope.AddFunc("has", "has(key)", f);
			}
			{
				auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					X::ARGS& params,
					X::KWARGS& kwParams,
					X::Value& retValue)
					{
						Dict* pObj = dynamic_cast<Dict*>(pContext);
						retValue = X::Value(pObj->Remove(params[0]));
						return true;
					};
				_dictScope.AddFunc("remove", "remove(key)", f);
			}
			{
				auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					X::ARGS& params,
					X::KWARGS& kwParams,
					X::Value& retValue)
					{
						Dict* pObj = dynamic_cast<Dict*>(pContext);
						pObj->Set(params[0], params[1]);
						retValue = true;
						return true;
					};
				_dictScope.AddFunc("set", "set(key,val)", f);
			}
			{
				auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					X::ARGS& params,
					X::KWARGS& kwParams,
					X::Value& retValue)
					{
						if (params.size() == 0)
						{
							return true;
						}
						Dict* pObj = dynamic_cast<Dict*>(pContext);
						retValue.Clear();
						pObj->Get(params[0], retValue);
						if (retValue.IsInvalid() && params.size()>1)
						{
							retValue = params[1];
						}
						//we don't need to care if find it or not
						//always make it success, so xlang run statement is OK
						return true;
					};
				_dictScope.AddFunc("get", "val = get(key)", f);
			}
			{
				auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					X::ARGS& params,
					X::KWARGS& kwParams,
					X::Value& retValue)
					{
						Dict* pObj = dynamic_cast<Dict*>(pContext);
						X::List list;
						for (auto& it : pObj->mMap)
						{
							list += it.first;
						}
						retValue = list;
						return true;
					};
				_dictScope.AddFunc("keys", "key_list = dict.keys()", f);
			}
			{
				auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					X::ARGS& params,
					X::KWARGS& kwParams,
					X::Value& retValue)
					{
						Dict* pObj = dynamic_cast<Dict*>(pContext);
						X::List list;
						for (auto& it : pObj->mMap)
						{
							list += it.second;
						}
						retValue = list;
						return true;
					};
				_dictScope.AddFunc("values", "value_list = dict.values()", f);
			}
			_dictScope.Close();
		}
		void Dict::cleanup()
		{
			_dictScope.Clean();
		}
		Dict::Dict() :XDict(0)
		{
			m_t = ObjType::Dict;
			m_bases.push_back(_dictScope.GetMyScope());
		}
		bool Dict::GetLValueToAssign(X::Value& key, X::Value& value)
		{
			InternalAssign* pAssign = new InternalAssign(this, key);
			value = X::Value(pAssign);
			return true;
		}
		List* Dict::FlatPack(XlangRuntime* rt, XObj* pContext,
			std::vector<std::string>& IdList, int id_offset,
			long long startIndex, long long count)
		{
			AutoLock autoLock(m_lock);
			if (id_offset < IdList.size())
			{
				Value key(IdList[id_offset++]);
				Value item;
				bool bOK = Get(key, item);
				if (!bOK)
				{
					//TODO:non-string key, need to use pointer->XObj* to lookup

				}
				if (item.IsObject())
				{
					Object* pChildObj = dynamic_cast<Object*>(item.GetObj());
					if (pChildObj)
					{
						return pChildObj->FlatPack(rt, pContext, IdList, id_offset, startIndex, count);
					}
				}
				//all elses, return an empty list
				List* pOutList = new List();
				pOutList->IncRef();
				return pOutList;
			}
			List* pOutList = new List();
			pOutList->IncRef();
			for (auto& it: mMap)
			{
				Dict* dict = new Dict();
				std::string myId;
				X::Value key = it.first;
				if (!key.IsObject() || (key.IsObject() && 
					dynamic_cast<Object*>(key.GetObj())->IsStr()))
				{
					dict->Set("Name", key);
					myId = key.ToString();
				}
				else if(key.IsObject())
				{
					auto ptrId = (unsigned long long)key.GetObj();
					myId = "ptr:" + ::tostring(ptrId);
					X::Value objId(ptrId);
					dict->Set("Name", objId);
				}
				//make object_ids
				auto objIds = CombinObjectIds(IdList,myId);
				dict->Set("Id", objIds);

				X::Value val = it.second;
				auto valType = val.GetValueType();
				Data::Str* pStrType = new Data::Str(valType);
				dict->Set("Type", X::Value(pStrType));
				if (!val.IsObject() || (val.IsObject() && 
					dynamic_cast<Object*>(val.GetObj())->IsStr()))
				{
					dict->Set("Value", val);
				}
				else if (val.IsObject())
				{
					X::Value objId((unsigned long long)val.GetObj());
					dict->Set("Value", objId);
					X::Value valShape = val.GetObj()->Shapes();
					if (valShape.IsList())
					{
						dict->Set("Size", valShape);
					}
					else
					{
						X::Value valSize(val.GetObj()->Size());
						dict->Set("Size", valSize);
					}
				}
				X::Value valDict(dict);
				pOutList->Add(rt, valDict);
			}
			return pOutList;
		}
		X::Value Dict::UpdateItemValue(XlangRuntime* rt, XObj* pContext,
			std::vector<std::string>& IdList, int id_offset,
			std::string itemName, X::Value& val)
		{
			AutoLock autoLock(m_lock);
			if (id_offset < IdList.size())
			{
				Value key(IdList[id_offset++]);
				Value item;
				bool bOK = Get(key, item);
				if (!bOK)
				{
					//TODO:non-string key, need to use pointer->XObj* to lookup

				}
				if (item.IsObject())
				{
					Object* pChildObj = dynamic_cast<Object*>(item.GetObj());
					if (pChildObj)
					{
						return pChildObj->UpdateItemValue(rt, pContext,
							IdList, id_offset, itemName, val);
					}
				}
				return val;//all elses, no change
			}
			X::Value itemKey(itemName);
			Set(itemKey, val);
			return val;
		}
	}
}