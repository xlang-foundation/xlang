#include "dict.h"
#include "list.h"
#include "port.h"
#include "function.h"
#include "obj_func_scope.h"

namespace X
{
	namespace Data
	{
		static Obj_Func_Scope<1> _dictScope;
		void Dict::Init()
		{
			_dictScope.Init();
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
		class DictValue :
			public Value
		{
			Dict* m_dict = nullptr;
			Value m_key;
		public:
			DictValue(Dict* d, Value& k)
			{
				d->IncRef();
				SetObj(d);
				m_key = k;
				m_dict = d;
			}
			inline void operator += (const Value& v)
			{
				m_dict->AddKeyValue(m_key, v);
			}
			virtual inline void operator = (const Value& v) override
			{
				m_dict->SetKV(m_key, v);
			}
		};
		void Dict::HookLValue(X::Value& key,X::LValue* lValue)
		{
			auto* pDictVal = new DictValue(this, key);
			*lValue = pDictVal;
			lValue->SetReleaseFlag(true);
		}
	}
}