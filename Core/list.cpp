#include "list.h"
#include "utility.h"
#include "dict.h"
#include "port.h"
#include "function.h"

namespace X
{
	namespace Data
	{
		bool List::Call(XRuntime* rt, XObj* pContext,
			ARGS& params,
			KWARGS& kwParams,
			X::Value& retValue)
		{
			//do twice, first to do size or other call with
			//memory allocation
			for (auto it : kwParams)
			{
				if (it.first == "size")
				{
					long long size = it.second.GetLongLong();
					AutoLock(m_lock);
					m_data.resize(size);
					break;
				}
			}
			for (auto it : kwParams)
			{
				if (it.first == "init")
				{
					auto v0 = it.second;
					if (v0.IsObject() 
						&& v0.GetObj()->GetType() ==ObjType::Str
						&& v0.ToString().find("rand")==0)
					{
						auto strV = v0.ToString();
						double d1 = 0;
						double d2 = 1;
						SCANF(strV.c_str(),"rand(%lf,%lf)",&d1,&d2);
						AutoLock(m_lock);
						for (auto& v : m_data)
						{
							v = randDouble(d1,d2);
						}
					}
					else
					{
						AutoLock(m_lock);
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
		List* List::FlatPack(Runtime* rt,long long startIndex, long long count)
		{
			AutoLock(m_lock);
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
			for (long long i = 0; i < count; i++)
			{
				long long idx = startIndex + i;
				X::Value val;
				Get(idx, val);
				Dict* dict = new Dict();
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
		void ListScope::Init()
		{
			m_stackFrame = new AST::StackFrame(this);
			m_stackFrame->SetVarCount(2);

			std::string strName;
			{
				strName = "remove";
				AST::ExternFunc* extFunc = new AST::ExternFunc(strName,
					(X::U_FUNC)([](X::XRuntime* rt, XObj* pContext,
						X::ARGS& params,
						X::KWARGS& kwParams,
						X::Value& retValue)
						{
							List* pObj = dynamic_cast<List*>(pContext);
							long long idx = params[0];
							pObj->Remove(idx);
							retValue = Value(true);
							return true;
						}));
				auto* pFuncObj = new Function(extFunc);
				pFuncObj->IncRef();
				int idx = AddOrGet(strName, false);
				Value funcVal(pFuncObj);
				m_stackFrame->Set(idx, funcVal);
			}
			{
				strName = "size";
				AST::ExternFunc* extFunc = new AST::ExternFunc(strName,
					(X::U_FUNC)([](X::XRuntime* rt, XObj* pContext,
						X::ARGS& params,
						X::KWARGS& kwParams,
						X::Value& retValue)
						{
							std::cout << "List.Size" << std::endl;
							List* pObj = dynamic_cast<List*>(pContext);
							retValue = Value(pObj->Size());
							std::cout << "List.Size->End" << std::endl;
							return true;
						}));
				auto* pFuncObj = new Function(extFunc);
				pFuncObj->IncRef();
				int idx = AddOrGet(strName, false);
				Value funcVal(pFuncObj);
				m_stackFrame->Set(idx, funcVal);
			}

		}
		AST::Scope* ListScope::GetParentScope()
		{
			return nullptr;
		}
	}
}