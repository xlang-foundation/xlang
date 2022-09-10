#include "xclass_object.h"
#include "list.h"
#include "dict.h"

namespace X
{
	namespace Data
	{
		List* XClassObject::FlatPack(Runtime* rt,
			long long startIndex, long long count)
		{
			List* pOutList = new List();
			AST::Scope* pMyScope = dynamic_cast<AST::Scope*>(m_obj);
			auto vars = pMyScope->GetVarMap();
			for (auto& it : vars)
			{
				X::Value val;
				pMyScope->Get(rt, this, it.second, val);
				Dict* dict = new Dict();
				Data::Str* pStrName = new Data::Str(it.first);
				dict->Set("Name", X::Value(pStrName));
				auto valType = val.GetValueType();
				Data::Str* pStrType = new Data::Str(valType);
				dict->Set("Type", X::Value(pStrType));
				if (!val.IsObject() || (val.IsObject()
					&& dynamic_cast<Object*>(val.GetObj())->IsStr()))
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