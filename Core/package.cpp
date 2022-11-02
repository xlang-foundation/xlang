#include "package.h"
#include "list.h"
#include "dict.h"

namespace X
{
	namespace AST
	{
		X::Data::List* Package::FlatPack(XlangRuntime* rt,
			long long startIndex, long long count)
		{
			X::Data::List* pOutList = new X::Data::List();
			for (auto& it : m_memberInfos)
			{
				X::Data::Dict* dict = new X::Data::Dict();
				Data::Str* pStrName = new Data::Str(it.name);
				dict->Set("Name", X::Value(pStrName));
				int idx = Scope::AddOrGet(it.name, true);
				X::Value val;
				GetIndexValue(idx, val);
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
		X::Data::List* PackageProxy::FlatPack(XlangRuntime* rt,
			long long startIndex, long long count)
		{
			X::Data::List* pOutList = new X::Data::List();
			for (auto& it : m_pPackage->MemberInfos())
			{
				X::Data::Dict* dict = new X::Data::Dict();
				Data::Str* pStrName = new Data::Str(it.name);
				dict->Set("Name", X::Value(pStrName));
				int idx = AddOrGet(it.name, true);
				X::Value val;
				GetIndexValue(idx, val);
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