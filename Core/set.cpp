﻿/*
Copyright (C) 2024 The XLang Foundation
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "set.h"
#include "utility.h"
#include "dict.h"
#include "list.h"
#include "port.h"
#include "function.h"

namespace X
{
	namespace Data
	{
		class SetScope :
			virtual public AST::Scope
		{
			AST::StackFrame* m_stackFrame = nullptr;
		public:
			SetScope() :
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
			~SetScope()
			{
				if (m_stackFrame)
				{
					delete m_stackFrame;
				}
			}
			void Init()
			{
				m_stackFrame = new AST::StackFrame();
				m_stackFrame->SetVarCount(3);

				std::string strName;
				{
					strName = "remove";
					auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
						X::ARGS& params,
						X::KWARGS& kwParams,
						X::Value& retValue)
					{
						mSet* pObj = dynamic_cast<mSet*>(pContext);
						long long idx = params[0];
						pObj->Remove(idx);
						retValue = Value(true);
						return true;
					};
					X::U_FUNC func(f);
					AST::ExternFunc* extFunc = new AST::ExternFunc(strName,
						"remove(index)",func);
					auto* pFuncObj = new Function(extFunc);
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
						mSet* pObj = dynamic_cast<mSet*>(pContext);
						pObj->Clear();
						retValue = Value(true);
						return true;
					};
					X::U_FUNC func(f);
					AST::ExternFunc* extFunc = new AST::ExternFunc(strName,
						"clear()",func);
					auto* pFuncObj = new Function(extFunc);
					pFuncObj->IncRef();
					int idx = AddOrGet(strName, false);
					Value funcVal(pFuncObj);
					m_stackFrame->Set(idx, funcVal);
				}
				{
					strName = "size";
					auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
						X::ARGS& params,
						X::KWARGS& kwParams,
						X::Value& retValue)
					{
						std::cout << "Set.Size" << std::endl;
						mSet* pObj = dynamic_cast<mSet*>(pContext);
						retValue = Value(pObj->Size());
						std::cout << "Set.Size->End" << std::endl;
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
		};
		static SetScope* _SetScope = nullptr;
		void mSet::Init()
		{
			_SetScope = new SetScope();
		}
		void mSet::cleanup()
		{
			if (_SetScope)
			{
				_SetScope->clean();
				delete _SetScope;
				_SetScope = nullptr;
			}
		}
		mSet::mSet() :
			XSet(0),
			Object()
		{
			m_t = ObjType::Set;
			m_bases.push_back(_SetScope);

		}
		bool mSet::Call(XRuntime* rt, XObj* pContext,
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
		
		List* mSet::FlatPack(XlangRuntime* rt, XObj* pContext,
			std::vector<std::string>& IdSet, int id_offset,
			long long startIndex, long long count)
		{
			AutoLock autoLock(m_lock);
			if (id_offset < IdSet.size())
			{
				unsigned long long index = 0;
				SCANF(IdSet[id_offset++].c_str(), "%llu", &index);
				Value item;
				Get(index, item);
				if (item.IsObject())
				{
					Object* pChildObj = dynamic_cast<Object*>(item.GetObj());
					if (pChildObj)
					{
						return pChildObj->FlatPack(rt, pContext, IdSet, id_offset, startIndex, count);
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
				auto objIds = CombinObjectIds(IdSet, (unsigned long long)idx);
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
					X::Value valShape = val.GetObj()->Shapes();
					if (valShape.IsList())
					{
						dict->Set("Size", valShape);
					}
					else
					{
						X::Value valSize(val.GetObj()->Size());
						dict->Set("Size", valSize);
					}
				}
				X::Value valDict(dict);
				pOutList->Add(rt, valDict);
			}
			return pOutList;
		}
	}
}