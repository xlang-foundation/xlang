/*
Copyright (C) 2024 The XLang Foundation
Licensed under the Apache License, Version 2.0
*/

#include "set.h"
#include "utility.h"
#include "dict.h"
#include "list.h"
#include "port.h"
#include "function.h"
#include "obj_func_scope.h"

namespace X
{
	namespace Data
	{
		static Obj_Func_Scope<20> _setScope;

		void XlangSet::Init()
		{
			_setScope.Init();

			// add(item)
			{
				auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					X::ARGS& params, X::KWARGS& kwParams, X::Value& retValue)
				{
					XlangSet* pObj = dynamic_cast<XlangSet*>(pContext);
					if (params.size() >= 1) { X::Value v = params[0]; pObj->AddUnique(v); }
					retValue = X::Value(); return true;
				};
				_setScope.AddFunc("add", "add(item)", f);
			}

			// discard(item)
			{
				auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					X::ARGS& params, X::KWARGS& kwParams, X::Value& retValue)
				{
					XlangSet* pObj = dynamic_cast<XlangSet*>(pContext);
					if (params.size() >= 1) { X::Value v = params[0]; pObj->Remove_NoLock(v); }
					retValue = X::Value(); return true;
				};
				_setScope.AddFunc("discard", "discard(item)", f);
			}

			// remove(item)
			{
				auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					X::ARGS& params, X::KWARGS& kwParams, X::Value& retValue)
				{
					XlangSet* pObj = dynamic_cast<XlangSet*>(pContext);
					if (params.size() >= 1) { X::Value v = params[0]; pObj->Remove_NoLock(v); }
					retValue = X::Value(); return true;
				};
				_setScope.AddFunc("remove", "remove(item)", f);
			}

			// pop()
			{
				auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					X::ARGS& params, X::KWARGS& kwParams, X::Value& retValue)
				{
					XlangSet* pObj = dynamic_cast<XlangSet*>(pContext);
					AutoLock autoLock(pObj->m_lock);
					if (!pObj->m_data.empty())
					{
						retValue = pObj->m_data.back();
						pObj->m_data.pop_back();
					}
					else { retValue = X::Value(); }
					return true;
				};
				_setScope.AddFunc("pop", "pop()", f);
			}

			// clear()
			{
				auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					X::ARGS& params, X::KWARGS& kwParams, X::Value& retValue)
				{
					XlangSet* pObj = dynamic_cast<XlangSet*>(pContext);
					AutoLock autoLock(pObj->m_lock);
					pObj->m_data.clear();
					retValue = X::Value(); return true;
				};
				_setScope.AddFunc("clear", "clear()", f);
			}

			// copy()
			{
				auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					X::ARGS& params, X::KWARGS& kwParams, X::Value& retValue)
				{
					XlangSet* pObj = dynamic_cast<XlangSet*>(pContext);
					XlangSet* pCopy = dynamic_cast<XlangSet*>(pObj->Clone());
					retValue = X::Value(pCopy); return true;
				};
				_setScope.AddFunc("copy", "copy()", f);
			}

			// size()
			{
				auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					X::ARGS& params, X::KWARGS& kwParams, X::Value& retValue)
				{
					XlangSet* pObj = dynamic_cast<XlangSet*>(pContext);
					retValue = X::Value((long long)pObj->Size()); return true;
				};
				_setScope.AddFunc("size", "size()", f);
			}

			// count() - alias for size
			{
				auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					X::ARGS& params, X::KWARGS& kwParams, X::Value& retValue)
				{
					XlangSet* pObj = dynamic_cast<XlangSet*>(pContext);
					retValue = X::Value((long long)pObj->Size()); return true;
				};
				_setScope.AddFunc("count", "count()", f);
			}

			// update(*others) - in-place union
			{
				auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					X::ARGS& params, X::KWARGS& kwParams, X::Value& retValue)
				{
					XlangSet* pObj = dynamic_cast<XlangSet*>(pContext);
					for (size_t pi = 0; pi < params.size(); pi++)
					{
						X::Value other = params[pi];
						if (other.IsObject())
						{
							Object* pOther = dynamic_cast<Object*>(other.GetObj());
							if (pOther && pOther->GetType() == ObjType::Set)
							{
								XlangSet* pOtherSet = dynamic_cast<XlangSet*>(pOther);
								long long sz = pOtherSet->Size();
								for (long long i = 0; i < sz; i++)
								{
									X::Value v; pOtherSet->Get(i, v); pObj->AddUnique(v);
								}
							}
						}
					}
					retValue = X::Value(); return true;
				};
				_setScope.AddFunc("update", "update(*others)", f);
			}

			// union(other) - return new set
			{
				auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					X::ARGS& params, X::KWARGS& kwParams, X::Value& retValue)
				{
					XlangSet* pObj = dynamic_cast<XlangSet*>(pContext);
					XlangSet* pResult = dynamic_cast<XlangSet*>(pObj->Clone());
					for (size_t pi = 0; pi < params.size(); pi++)
					{
						X::Value other = params[pi];
						if (other.IsObject())
						{
							Object* pOther = dynamic_cast<Object*>(other.GetObj());
							if (pOther && pOther->GetType() == ObjType::Set)
							{
								XlangSet* pOtherSet = dynamic_cast<XlangSet*>(pOther);
								long long sz = pOtherSet->Size();
								for (long long i = 0; i < sz; i++)
								{
									X::Value v; pOtherSet->Get(i, v); pResult->AddUnique(v);
								}
							}
						}
					}
					retValue = X::Value(pResult); return true;
				};
				_setScope.AddFunc("union", "union(other)", f);
			}

			// intersection(other) - return new set
			{
				auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					X::ARGS& params, X::KWARGS& kwParams, X::Value& retValue)
				{
					XlangSet* pObj = dynamic_cast<XlangSet*>(pContext);
					XlangSet* pResult = new XlangSet();
					pResult->IncRef();
					if (params.size() >= 1)
					{
						X::Value other = params[0];
						if (other.IsObject())
						{
							Object* pOther = dynamic_cast<Object*>(other.GetObj());
							if (pOther && pOther->GetType() == ObjType::Set)
							{
								XlangSet* pOtherSet = dynamic_cast<XlangSet*>(pOther);
								long long sz = pObj->Size();
								for (long long i = 0; i < sz; i++)
								{
									X::Value v; pObj->Get(i, v);
									if (pOtherSet->IsContain(v)) { pResult->AddUnique(v); }
								}
							}
						}
					}
					retValue = X::Value(pResult); return true;
				};
				_setScope.AddFunc("intersection", "intersection(other)", f);
			}

			// intersection_update(other) - in-place
			{
				auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					X::ARGS& params, X::KWARGS& kwParams, X::Value& retValue)
				{
					XlangSet* pObj = dynamic_cast<XlangSet*>(pContext);
					if (params.size() >= 1)
					{
						X::Value other = params[0];
						if (other.IsObject())
						{
							Object* pOther = dynamic_cast<Object*>(other.GetObj());
							if (pOther && pOther->GetType() == ObjType::Set)
							{
								XlangSet* pOtherSet = dynamic_cast<XlangSet*>(pOther);
								AutoLock autoLock(pObj->m_lock);
								std::vector<X::Value> kept;
								for (auto& v : pObj->m_data)
								{
									if (pOtherSet->IsContain(v)) { kept.push_back(v); }
								}
								pObj->m_data = std::move(kept);
							}
						}
					}
					retValue = X::Value(); return true;
				};
				_setScope.AddFunc("intersection_update", "intersection_update(other)", f);
			}

			// difference(other) - return new set
			{
				auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					X::ARGS& params, X::KWARGS& kwParams, X::Value& retValue)
				{
					XlangSet* pObj = dynamic_cast<XlangSet*>(pContext);
					XlangSet* pResult = new XlangSet();
					pResult->IncRef();
					long long sz = pObj->Size();
					for (long long i = 0; i < sz; i++)
					{
						X::Value v; pObj->Get(i, v);
						bool inOther = false;
						for (size_t pi = 0; pi < params.size() && !inOther; pi++)
						{
							X::Value other = params[pi];
							if (other.IsObject())
							{
								Object* pOther = dynamic_cast<Object*>(other.GetObj());
								if (pOther && pOther->GetType() == ObjType::Set)
								{
									XlangSet* pOtherSet = dynamic_cast<XlangSet*>(pOther);
									if (pOtherSet->IsContain(v)) { inOther = true; }
								}
							}
						}
						if (!inOther) { pResult->AddUnique(v); }
					}
					retValue = X::Value(pResult); return true;
				};
				_setScope.AddFunc("difference", "difference(other)", f);
			}

			// difference_update(other) - in-place
			{
				auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					X::ARGS& params, X::KWARGS& kwParams, X::Value& retValue)
				{
					XlangSet* pObj = dynamic_cast<XlangSet*>(pContext);
					for (size_t pi = 0; pi < params.size(); pi++)
					{
						X::Value other = params[pi];
						if (other.IsObject())
						{
							Object* pOther = dynamic_cast<Object*>(other.GetObj());
							if (pOther && pOther->GetType() == ObjType::Set)
							{
								XlangSet* pOtherSet = dynamic_cast<XlangSet*>(pOther);
								long long sz = pOtherSet->Size();
								for (long long i = 0; i < sz; i++)
								{
									X::Value v; pOtherSet->Get(i, v); pObj->Remove_NoLock(v);
								}
							}
						}
					}
					retValue = X::Value(); return true;
				};
				_setScope.AddFunc("difference_update", "difference_update(other)", f);
			}

			// symmetric_difference(other) - return new set
			{
				auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					X::ARGS& params, X::KWARGS& kwParams, X::Value& retValue)
				{
					XlangSet* pObj = dynamic_cast<XlangSet*>(pContext);
					XlangSet* pResult = new XlangSet();
					pResult->IncRef();
					if (params.size() >= 1)
					{
						X::Value other = params[0];
						if (other.IsObject())
						{
							Object* pOther = dynamic_cast<Object*>(other.GetObj());
							if (pOther && pOther->GetType() == ObjType::Set)
							{
								XlangSet* pOtherSet = dynamic_cast<XlangSet*>(pOther);
								long long sz = pObj->Size();
								for (long long i = 0; i < sz; i++)
								{
									X::Value v; pObj->Get(i, v);
									if (!pOtherSet->IsContain(v)) { pResult->AddUnique(v); }
								}
								sz = pOtherSet->Size();
								for (long long i = 0; i < sz; i++)
								{
									X::Value v; pOtherSet->Get(i, v);
									if (!pObj->IsContain(v)) { pResult->AddUnique(v); }
								}
							}
						}
					}
					retValue = X::Value(pResult); return true;
				};
				_setScope.AddFunc("symmetric_difference", "symmetric_difference(other)", f);
			}

			// symmetric_difference_update(other) - in-place
			{
				auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					X::ARGS& params, X::KWARGS& kwParams, X::Value& retValue)
				{
					XlangSet* pObj = dynamic_cast<XlangSet*>(pContext);
					if (params.size() >= 1)
					{
						X::Value other = params[0];
						if (other.IsObject())
						{
							Object* pOther = dynamic_cast<Object*>(other.GetObj());
							if (pOther && pOther->GetType() == ObjType::Set)
							{
								XlangSet* pOtherSet = dynamic_cast<XlangSet*>(pOther);
								long long sz = pOtherSet->Size();
								std::vector<X::Value> toAdd;
								for (long long i = 0; i < sz; i++)
								{
									X::Value v; pOtherSet->Get(i, v);
									if (!pObj->IsContain(v)) { toAdd.push_back(v); }
								}
								for (long long i = 0; i < sz; i++)
								{
									X::Value v; pOtherSet->Get(i, v);
									if (pObj->IsContain(v)) { pObj->Remove_NoLock(v); }
								}
								for (auto& v : toAdd) { X::Value vv = v; pObj->AddUnique(vv); }
							}
						}
					}
					retValue = X::Value(); return true;
				};
				_setScope.AddFunc("symmetric_difference_update",
					"symmetric_difference_update(other)", f);
			}

			// issubset(other) - return bool
			{
				auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					X::ARGS& params, X::KWARGS& kwParams, X::Value& retValue)
				{
					XlangSet* pObj = dynamic_cast<XlangSet*>(pContext);
					bool result = false;
					if (params.size() >= 1)
					{
						X::Value other = params[0];
						if (other.IsObject())
						{
							Object* pOther = dynamic_cast<Object*>(other.GetObj());
							if (pOther && pOther->GetType() == ObjType::Set)
							{
								XlangSet* pOtherSet = dynamic_cast<XlangSet*>(pOther);
								result = true;
								long long sz = pObj->Size();
								for (long long i = 0; i < sz; i++)
								{
									X::Value v; pObj->Get(i, v);
									if (!pOtherSet->IsContain(v)) { result = false; break; }
								}
							}
						}
					}
					retValue = X::Value(result); return true;
				};
				_setScope.AddFunc("issubset", "issubset(other)", f);
			}

			// issuperset(other) - return bool
			{
				auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					X::ARGS& params, X::KWARGS& kwParams, X::Value& retValue)
				{
					XlangSet* pObj = dynamic_cast<XlangSet*>(pContext);
					bool result = false;
					if (params.size() >= 1)
					{
						X::Value other = params[0];
						if (other.IsObject())
						{
							Object* pOther = dynamic_cast<Object*>(other.GetObj());
							if (pOther && pOther->GetType() == ObjType::Set)
							{
								XlangSet* pOtherSet = dynamic_cast<XlangSet*>(pOther);
								result = true;
								long long sz = pOtherSet->Size();
								for (long long i = 0; i < sz; i++)
								{
									X::Value v; pOtherSet->Get(i, v);
									if (!pObj->IsContain(v)) { result = false; break; }
								}
							}
						}
					}
					retValue = X::Value(result); return true;
				};
				_setScope.AddFunc("issuperset", "issuperset(other)", f);
			}

			// isdisjoint(other) - return bool
			{
				auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					X::ARGS& params, X::KWARGS& kwParams, X::Value& retValue)
				{
					XlangSet* pObj = dynamic_cast<XlangSet*>(pContext);
					bool result = true;
					if (params.size() >= 1)
					{
						X::Value other = params[0];
						if (other.IsObject())
						{
							Object* pOther = dynamic_cast<Object*>(other.GetObj());
							if (pOther && pOther->GetType() == ObjType::Set)
							{
								XlangSet* pOtherSet = dynamic_cast<XlangSet*>(pOther);
								long long sz = pObj->Size();
								for (long long i = 0; i < sz; i++)
								{
									X::Value v; pObj->Get(i, v);
									if (pOtherSet->IsContain(v)) { result = false; break; }
								}
							}
						}
					}
					retValue = X::Value(result); return true;
				};
				_setScope.AddFunc("isdisjoint", "isdisjoint(other)", f);
			}

			_setScope.Close();
		}

		void XlangSet::cleanup()
		{
			_setScope.Clean();
		}

		XlangSet::XlangSet() :
			XSet(0),
			Object()
		{
			m_t = ObjType::Set;
			m_bases.push_back(_setScope.GetMyScope());
		}

		bool XlangSet::Call(XRuntime* rt, XObj* pContext,
			ARGS& params, KWARGS& kwParams, X::Value& retValue)
		{
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
						&& v0.GetObj()->GetType() == ObjType::Str
						&& v0.ToString().find("rand") == 0)
					{
						auto strV = v0.ToString();
						double d1 = 0, d2 = 1;
						SCANF(strV.c_str(), "rand(%lf,%lf)", &d1, &d2);
						AutoLock autoLock(m_lock);
						for (auto& v : m_data) { v = randDouble(d1, d2); }
					}
					else
					{
						AutoLock autoLock(m_lock);
						for (auto& v : m_data) { v = v0; }
					}
					break;
				}
			}
			retValue = X::Value(this);
			return true;
		}

		List* XlangSet::FlatPack(XlangRuntime* rt, XObj* pContext,
			std::vector<std::string>& IdSet, int id_offset,
			long long startIndex, long long count)
		{
			AutoLock autoLock(m_lock);
			if (id_offset < (int)IdSet.size())
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
						return pChildObj->FlatPack(rt, pContext, IdSet, id_offset,
							startIndex, count);
					}
				}
				List* pOutList = new List();
				pOutList->IncRef();
				return pOutList;
			}
			if (startIndex < 0 || startIndex >= Size()) { return nullptr; }
			if (count == -1) { count = Size() - startIndex; }
			if ((startIndex + count) > Size()) { return nullptr; }
			List* pOutList = new List();
			pOutList->IncRef();
			for (long long i = 0; i < count; i++)
			{
				long long idx = startIndex + i;
				X::Value val;
				Get(idx, val);
				Dict* dict = new Dict();
				auto objIds = CombinObjectIds(IdSet, (unsigned long long)idx);
				dict->Set(X::Value(new Str(objIds.c_str(), (int)objIds.size())),
					X::Value(new Str("Id", 2)));
				auto valType = val.GetValueType();
				Data::Str* pStrType = new Data::Str(valType);
				dict->Set(X::Value(new Str("Type", 4)), X::Value(pStrType));
				if (!val.IsObject() ||
					(val.IsObject() && dynamic_cast<Object*>(val.GetObj())->IsStr()))
				{
					dict->Set(X::Value(new Str("Value", 5)), val);
				}
				else if (val.IsObject())
				{
					X::Value objId((unsigned long long)val.GetObj());
					dict->Set(X::Value(new Str("Value", 5)), objId);
					X::Value valShape = val.GetObj()->Shapes();
					if (valShape.IsList())
					{
						dict->Set(X::Value(new Str("Size", 4)), valShape);
					}
					else
					{
						X::Value valSize(val.GetObj()->Size());
						dict->Set(X::Value(new Str("Size", 4)), valSize);
					}
				}
				X::Value valDict(dict);
				pOutList->Add(rt, valDict);
			}
			return pOutList;
		}
	}
}