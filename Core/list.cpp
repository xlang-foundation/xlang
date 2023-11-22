#include "list.h"
#include "utility.h"
#include "dict.h"
#include "port.h"
#include "function.h"
#include "obj_func_scope.h"

namespace X
{
	namespace Data
	{
		static Obj_Func_Scope<3> _listScope;
		void List::Init()
		{
			_listScope.Init();
			{
				auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					X::ARGS& params,
					X::KWARGS& kwParams,
					X::Value& retValue)
				{
					List* pObj = dynamic_cast<List*>(pContext);
					long long idx = params[0];
					pObj->Remove(idx);
					retValue = Value(true);
					return true;
				};
				_listScope.AddFunc("remove", "remove(index)", f);
			}
			{
				auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					X::ARGS& params,
					X::KWARGS& kwParams,
					X::Value& retValue)
				{
					List* pObj = dynamic_cast<List*>(pContext);
					pObj->Clear();
					retValue = Value(true);
					return true;
				};
				_listScope.AddFunc("clear", "clear()", f);
			}
			{
				auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					X::ARGS& params,
					X::KWARGS& kwParams,
					X::Value& retValue)
				{
					List* pObj = dynamic_cast<List*>(pContext);
					retValue = Value(pObj->Size());
					return true;
				};
				_listScope.AddFunc("size", "size()", f);
			}
		}
		void List::cleanup()
		{
			_listScope.Clean();
		}
		List::List() :
			XList(0),
			Object()
		{
			m_t = ObjType::List;
			m_bases.push_back(_listScope.GetMyScope());

		}
		bool List::Call(XRuntime* rt, XObj* pContext,
			ARGS& params,
			KWARGS& kwParams,
			X::Value& retValue)
		{
			//do twice, first to do size or other call with
			//memory allocation
			for (auto it : kwParams)
			{
				if (it.Match("size"))
				{
					long long size = it.val.GetLongLong();
					AutoLock autoLock(m_lock);
					m_data.resize(size);
					break;
				}
			}
			for (auto it : kwParams)
			{
				if (it.Match("init"))
				{
					auto v0 = it.val;
					if (v0.IsObject() 
						&& v0.GetObj()->GetType() ==ObjType::Str
						&& v0.ToString().find("rand")==0)
					{
						auto strV = v0.ToString();
						double d1 = 0;
						double d2 = 1;
						SCANF(strV.c_str(),"rand(%lf,%lf)",&d1,&d2);
						AutoLock autoLock(m_lock);
						for (auto& v : m_data)
						{
							v = randDouble(d1,d2);
						}
					}
					else
					{
						AutoLock autoLock(m_lock);
						for (auto& v : m_data)
						{
							v = v0;
						}
					}
					break;
				}
			}
			retValue = X::Value(this);
			return true;
		}
		X::Value List::UpdateItemValue(XlangRuntime* rt, XObj* pContext,
			std::vector<std::string>& IdList, int id_offset,
			std::string itemName, X::Value& val)
		{
			AutoLock autoLock(m_lock);
			if (id_offset < IdList.size())
			{
				unsigned long long index = 0;
				SCANF(IdList[id_offset++].c_str(), "%llu", &index);
				Value item;
				Get(index, item);
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
			unsigned long long index = 0;
			SCANF(itemName.c_str(), "%llu", &index);
			Set(index, val);
			return val;
		}
		List* List::FlatPack(XlangRuntime* rt, XObj* pContext,
			std::vector<std::string>& IdList, int id_offset,
			long long startIndex, long long count)
		{
			AutoLock autoLock(m_lock);
			if (id_offset < IdList.size())
			{
				unsigned long long index = 0;
				SCANF(IdList[id_offset++].c_str(), "%llu", &index);
				Value item;
				Get(index, item);
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
			if (startIndex < 0 || startIndex >= Size())
			{
				return nullptr;
			}
			if (count == -1)
			{
				count = Size()- startIndex;
			}
			if ((startIndex + count) > Size())
			{
				return nullptr;
			}
			List* pOutList = new List();
			pOutList->IncRef();
			for (long long i = 0; i < count; i++)
			{
				long long idx = startIndex + i;
				X::Value val;
				Get(idx, val);
				Dict* dict = new Dict();
				auto objIds = CombinObjectIds(IdList, (unsigned long long)idx);
				dict->Set("Id", objIds);
				//Data::Str* pStrName = new Data::Str(it.first);
				//dict->Set("Name", X::Value(pStrName));
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