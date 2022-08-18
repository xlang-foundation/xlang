#include "dict.h"
#include "list.h"

namespace X
{
	namespace Data
	{
		List* Dict::FlatPack(Runtime* rt, long long startIndex, long long count)
		{
			List* pOutList = new List();
			for (auto& it: mMap)
			{
				Dict* dict = new Dict();
				X::Value key = it.first;
				if (!key.IsObject() || (key.IsObject() && 
					dynamic_cast<Object*>(key.GetObj())->IsStr()))
				{
					dict->Set("Name", key);
				}
				else if(key.IsObject())
				{//TODO: add ObjectId as New Value Type
					X::Value objId((unsigned long long)key.GetObj());
					dict->Set("Name", objId);
				}
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
	}
}