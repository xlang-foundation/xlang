/*
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
		static Obj_Func_Scope<12> _listScope;

		void List::Init()
		{
			_listScope.Init();

			// Remove function
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

			// Clear function
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

			// Append function
			{
				auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					X::ARGS& params,
					X::KWARGS& kwParams,
					X::Value& retValue)
					{
						List* pObj = dynamic_cast<List*>(pContext);
						for (auto& item : params)
						{
							pObj->Add((XlangRuntime*)rt, item);
						}
						retValue = Value(true);
						return true;
					};
				_listScope.AddFunc("append", "append()", f);
			}

			// Size function
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
				_listScope.AddFunc("count", "count()", f);
			}

			// Insert function
			{
				auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					X::ARGS& params,
					X::KWARGS& kwParams,
					X::Value& retValue)
					{
						List* pObj = dynamic_cast<List*>(pContext);
						long long idx = params[0];
						X::Value item = params[1];
						pObj->Insert(idx, (XlangRuntime*)rt, item);
						retValue = Value(true);
						return true;
					};
				_listScope.AddFunc("insert", "insert(index, item)", f);
			}

			// Pop function
			{
				auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					X::ARGS& params,
					X::KWARGS& kwParams,
					X::Value& retValue)
					{
						List* pObj = dynamic_cast<List*>(pContext);
						long long idx = params.size() > 0 ? (long long)params[0] : (pObj->Size() - 1);
						retValue = pObj->Get(idx);
						pObj->Remove(idx);
						return true;
					};
				_listScope.AddFunc("pop", "pop([index])", f);
			}
			// Index function
			{
				auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					X::ARGS& params,
					X::KWARGS& kwParams,
					X::Value& retValue)
					{
						List* pObj = dynamic_cast<List*>(pContext);
						X::Value item = params[0];
						long long index = -1;  // Default to -1 if not found

						for (long long i = 0; i < pObj->Size(); ++i) 
						{
							if (pObj->Get(i) == item) 
							{
								index = i;
								break;
							}
						}

						retValue = Value(index);
						return true;
					};
				_listScope.AddFunc("index", "index(item)", f);
			}

			// Reverse function
			{
				auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					X::ARGS& params,
					X::KWARGS& kwParams,
					X::Value& retValue)
					{
						List* pObj = dynamic_cast<List*>(pContext);

						// Create a temporary list to hold the reversed items
						List* temp = new List();
						for (long long i = pObj->Size() - 1; i >= 0; --i) {
							temp->Add(pObj->Get(i));
						}

						// Clear the original list and refill it with items from the temporary list
						pObj->Clear();
						for (long long i = 0; i < temp->Size(); ++i) {
							pObj->Add(temp->Get(i));
						}

						delete temp;  // Clean up the temporary list
						retValue = Value(true);
						return true;
					};
				_listScope.AddFunc("reverse", "reverse()", f);
			}

			// Copy function
			{
				auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					X::ARGS& params,
					X::KWARGS& kwParams,
					X::Value& retValue)
					{
						List* pObj = dynamic_cast<List*>(pContext);

						// Create a new list to be the copy
						List* copy = new List();
						for (long long i = 0; i < pObj->Size(); ++i) {
							copy->Add(pObj->Get(i));  // Copy each element
						}

						retValue = Value(copy);
						return true;
					};
				_listScope.AddFunc("copy", "copy()", f);
			}

			// Sort function with custom comparator support
			{
				auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					X::ARGS& params,
					X::KWARGS& kwParams,
					X::Value& retValue)
					{
						List* pObj = dynamic_cast<List*>(pContext);
						bool ascending = true;  // Default sort order is ascending
						X::Value customSortFunc;
						bool hasCustomFunc = false;

						// Check for custom sort function in kwargs
						auto it = kwParams.find("sortfunc");
						if (it) {
							customSortFunc = it->val;
							hasCustomFunc = true;
						}

						// Check for ascending parameter
						if (params.size() > 0 && params[0].IsBool()) {
							ascending = (bool)params[0];
						}

						// Selection sort implementation with custom comparator support
						for (long long i = 0; i < pObj->Size() - 1; ++i) {
							long long min_index = i;
							for (long long j = i + 1; j < pObj->Size(); ++j) {
								bool shouldSwap = false;

								if (hasCustomFunc) {
									// Use custom comparison function
									X::ARGS compareArgs(2);
									compareArgs.push_back(pObj->Get(j));
									compareArgs.push_back(pObj->Get(min_index));
									X::KWARGS compareKwArgs;
									X::Value compareResult;

									// Call the custom comparison function
									// Assuming the custom function returns a boolean or comparable value
									if (customSortFunc.IsObject()) {
										XObj* funcObj = customSortFunc.GetObj();
										if (funcObj && funcObj->Call(rt, pThis, compareArgs, 
											compareKwArgs, compareResult)) {
											if (compareResult.IsBool()) {
												shouldSwap = (bool)compareResult;
											}
											else if (compareResult.IsNumber()) {
												// If function returns number, negative means first < second
												shouldSwap = ascending ? 
													(compareResult.ToInt() < 0) : 
													(compareResult.ToInt() > 0);
											}
										}
									}
								}
								else {
									// Use default comparison
									shouldSwap = ascending ? 
										pObj->Get(j) < pObj->Get(min_index) : 
										pObj->Get(j) > pObj->Get(min_index);
								}

								if (shouldSwap) {
									min_index = j;
								}
							}

							// Swap elements if needed
							if (min_index != i) {
								X::Value temp = pObj->Get(min_index);
								X::Value temp2 = pObj->Get(i);
								pObj->Set(min_index, temp2);
								pObj->Set(i, temp);
							}
						}

						retValue = Value(true);
						return true;
					};
				_listScope.AddFunc("sort", "sort([ascending], sortfunc=function)", f);
			}

			// Extend function
			{
				auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					X::ARGS& params,
					X::KWARGS& kwParams,
					X::Value& retValue)
					{
						X::Value other = params[0];
						bool bOK = false;
						if (other.IsList())
						{
							List* pObj = dynamic_cast<List*>(pContext);
							List* otherObj = dynamic_cast<List*>(other.GetObj());

							// Add each element from the other list to the current list
							for (long long i = 0; i < otherObj->Size(); ++i)
							{
								pObj->Add(otherObj->Get(i));
							}
							bOK = true;
						}
						retValue = Value(bOK);
						return true;
					};
				_listScope.AddFunc("extend", "extend(other)", f);
			}
			_listScope.Close();
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