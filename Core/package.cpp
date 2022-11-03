#include "package.h"
#include "list.h"
#include "dict.h"
#include "prop.h"
#include "xpackage.h"

namespace X
{
	namespace AST
	{
		std::string GetMemberType(auto type)
		{
			std::string valType;
			switch (type)
			{
			case APISetBase::MemberType::Func:
			case APISetBase::MemberType::FuncEx:
				valType = "Function";
				break;
			case APISetBase::MemberType::Class:
			case APISetBase::MemberType::ClassInstance:
				valType = "Class";
				break;
			case APISetBase::MemberType::Prop:
				valType = "Prop";
				break;
			case APISetBase::MemberType::ObjectEvent:
				valType = "Event";
				break;
			default:
				break;
			}
			return valType;
		}
		X::Data::List* Package::FlatPack(XlangRuntime* rt,
			long long startIndex, long long count)
		{
			X::Data::List* pOutList = new X::Data::List();
			if (m_pAPISet == nullptr)
			{
				return pOutList;
			}
			auto& members = m_pAPISet->Members();
			for (auto& it : members)
			{
				X::Data::Dict* dict = new X::Data::Dict();
				Data::Str* pStrName = new Data::Str(it.name);
				dict->Set("Name", X::Value(pStrName));
				auto valType = GetMemberType(it.type);
				int idx = Scope::AddOrGet(it.name, true);
				X::Value val;
				GetIndexValue(idx, val);
				if (val.IsObject() && val.GetObj()->GetType()
					== X::ObjType::Prop)
				{
					auto* pPropObj = dynamic_cast<X::Data::PropObject*>(val.GetObj());
					if (pPropObj)
					{
						pPropObj->GetProp(rt, this, val);
					}
				}

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
			auto* pAPISet = m_pPackage->GetAPISet();
			if (pAPISet == nullptr)
			{
				return pOutList;
			}
			auto& members = pAPISet->Members();
			for (auto& it : members)
			{
				X::Data::Dict* dict = new X::Data::Dict();
				Data::Str* pStrName = new Data::Str(it.name);
				dict->Set("Name", X::Value(pStrName));
				int idx = AddOrGet(it.name, true);
				X::Value val;
				GetIndexValue(idx, val);
				if (val.IsObject() && val.GetObj()->GetType() 
					== X::ObjType::Prop)
				{
					auto* pPropObj = dynamic_cast<X::Data::PropObject*>(val.GetObj());
					if (pPropObj)
					{
						pPropObj->GetProp(rt, this, val);
					}
				}
				auto valType = GetMemberType(it.type);
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