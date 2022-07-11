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
				AST::Value key = it.first;
				if (!key.IsObject() || (key.IsObject() && key.GetObj()->IsStr()))
				{
					dict->Set("Name", key);
				}
				else if(key.IsObject())
				{//TODO: add ObjectId as New Value Type
					AST::Value objId((unsigned long long)key.GetObj());
					dict->Set("Name", objId);
				}
				AST::Value val = it.second;
				auto valType = val.GetValueType();
				Data::Str* pStrType = new Data::Str(valType);
				dict->Set("Type", AST::Value(pStrType));
				if (!val.IsObject() || (val.IsObject() && val.GetObj()->IsStr()))
				{
					dict->Set("Value", val);
				}
				else if (val.IsObject())
				{
					AST::Value objId((unsigned long long)val.GetObj());
					dict->Set("Value", objId);
					AST::Value valSize(val.GetObj()->Size());
					dict->Set("Size", valSize);
				}
				AST::Value valDict(dict);
				pOutList->Add(rt, valDict);
			}
			return pOutList;
		}
	}
}