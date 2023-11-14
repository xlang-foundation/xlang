#include "list.h"
#include "utility.h"
#include "dict.h"
#include "port.h"
#include "function.h"

namespace X
{
	namespace Data
	{
		class ListScope :
			virtual public AST::Scope
		{
			AST::VariableFrame* m_stackFrame = nullptr;
		public:
			ListScope() :
				Scope()
			{
				Init();
			}
			void clean()
			{
				if (m_stackFrame)
				{
					delete m_stackFrame;
					m_stackFrame = nullptr;
				}
			}
			~ListScope()
			{
				if (m_stackFrame)
				{
					delete m_stackFrame;
				}
			}
			void Init()
			{
				m_stackFrame = new AST::VariableFrame(this);
				m_stackFrame->SetVarCount(3);
				std::string strName;
				{
					strName = "remove";
					auto f = [](X::XRuntime* rt,XObj* pThis,XObj* pContext,
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
					X::U_FUNC func(f);
					AST::ExternFunc* extFunc = new AST::ExternFunc(strName, "remove(index)", func);
					auto* pFuncObj = new X::Data::Function(extFunc);
					pFuncObj->IncRef();
					int idx = AddOrGet(strName, false);
					Value funcVal(pFuncObj);
					m_stackFrame->Set(idx, funcVal);
				}
				{
					strName = "clear";
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
					X::U_FUNC func(f);
					AST::ExternFunc* extFunc = new AST::ExternFunc(strName,"clear()",func);
					auto* pFuncObj = new Function(extFunc);
					pFuncObj->IncRef();
					int idx = AddOrGet(strName, false);
					Value funcVal(pFuncObj);
					m_stackFrame->Set(idx, funcVal);
				}
				{
					strName = "size";
					auto f = [](X::XRuntime* rt,XObj* pThis,XObj* pContext,
						X::ARGS& params,
						X::KWARGS& kwParams,
						X::Value& retValue)
					{
						List* pObj = dynamic_cast<List*>(pContext);
						retValue = Value(pObj->Size());
						return true;
					};
					X::U_FUNC func(f);
					AST::ExternFunc* extFunc = new AST::ExternFunc(strName,
						"size()",func);
					auto* pFuncObj = new Function(extFunc);
					pFuncObj->IncRef();
					int idx = AddOrGet(strName, false);
					Value funcVal(pFuncObj);
					m_stackFrame->Set(idx, funcVal);
				}

			}
			// Inherited via Scope
			virtual Scope* GetParentScope() override
			{
				return nullptr;
			}
			virtual bool Set(XlangRuntime* rt, XObj* pContext, int idx, Value& v) override
			{
				m_stackFrame->Set(idx, v);
				return true;
			}
			virtual bool Get(XlangRuntime* rt, XObj* pContext, int idx, Value& v,
				LValue* lValue = nullptr) override
			{
				m_stackFrame->Get(idx, v, lValue);
				return true;
			}
		};
		static ListScope* _listScope = nullptr;
		void List::Init()
		{
			_listScope = new ListScope();
		}
		void List::cleanup()
		{
			if (_listScope)
			{
				_listScope->clean();
				delete _listScope;
				_listScope = nullptr;
			}
		}
		List::List() :
			XList(0),
			Object()
		{
			m_t = ObjType::List;
			m_bases.push_back(_listScope);

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