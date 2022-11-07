#include "dict.h"
#include "list.h"
#include "port.h"

namespace X
{
	namespace Data
	{
		List* Dict::FlatPack(XlangRuntime* rt, XObj* pContext,
			std::vector<std::string>& IdList, int id_offset,
			long long startIndex, long long count)
		{
			AutoLock(m_lock);
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
					X::Value valSize(val.GetObj()->Size());
					dict->Set("Size", valSize);
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
			AutoLock(m_lock);
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