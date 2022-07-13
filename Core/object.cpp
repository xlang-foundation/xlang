#include "object.h"
#include "list.h"
#include "dict.h"
#include "xclass.h"

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
				AST::Value val;
				pMyScope->Get(rt, this, it.second, val);
				Dict* dict = new Dict();
				Data::Str* pStrName = new Data::Str(it.first);
				dict->Set("Name", AST::Value(pStrName));
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