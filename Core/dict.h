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

#pragma once

#include "object.h"
#include <unordered_map>
namespace X
{
	namespace Data
	{
		class ObjectHashFunction
		{
		public:
			size_t operator()(const X::Value& key) const
			{
				X::Value v =key;
				return v.Hash();
			}
		};
		class Dict :
			virtual public XDict,
			virtual public Object
		{
			using Dict_MAP = std::unordered_map<X::Value,
				X::Value, ObjectHashFunction>;
		protected:
			std::vector<AST::Scope*> m_bases;
			Dict_MAP mMap;
		public:
			static void Init();
			static void cleanup();
			Dict();
			~Dict()
			{
				AutoLock autoLock(m_lock);
				mMap.clear();
				m_bases.clear();
			}
			FORCE_INLINE virtual long long Size() override
			{
				return mMap.size();
			}
			virtual X::Value Get(const X::Value& key) override
			{
				return mMap[key];
			}
			virtual Value Member(XRuntime* rt, const char* name) override
			{
				X::Value val;
				X::Value key(name);
				Get(key, val);
				return val;
			}
			virtual bool IsContain(X::Value& val) override
			{
				//search the key
				auto it = mMap.find(val);
				return it != mMap.end();
			}
			virtual Value& operator[](X::Value& key) override
			{
				return mMap[key];
			}
			virtual bool Set(X::Value valIdx, X::Value& val) override
			{
				auto it = mMap.find(valIdx);
				if (it != mMap.end())
				{
					it->second = val;
				}
				else
				{
					mMap.emplace(std::make_pair(valIdx, val));
				}
				return true;
			}
			virtual void Set(const X::Value& key, const X::Value& val) override
			{
				auto it = mMap.find(key);
				if (it != mMap.end())
				{
					it->second = val;
				}
				else
				{
					mMap.emplace(std::make_pair(key, val));
				}
			}
			void SetKV(X::Value& key,const X::Value& val)
			{
				mMap.emplace(std::make_pair(key, val));
			}
			virtual void GetBaseScopes(std::vector<AST::Scope*>& bases) override
			{
				Object::GetBaseScopes(bases);
				for (auto it : m_bases)
				{
					bases.push_back(it);
				}
			}
			void Set(const char* key, X::Value val)
			{
				mMap.emplace(std::make_pair(X::Value(key), val));
			}
			void AddKeyValue(Value& key, const Value& v)
			{
				auto it = mMap.find(key);
				if (it != mMap.end())
				{
					it->second += v;
				}
				else
				{
					mMap.emplace(std::make_pair(key, v));
				}
			}
			virtual Dict& operator +=(X::Value& r)
			{
				if (r.IsObject())
				{
					Object* pObj = dynamic_cast<Object*>(r.GetObj());
					if (pObj->GetType() == ObjType::Dict)
					{
						Dict* pDictOther = dynamic_cast<Dict*>(pObj);
						for (auto& it : pDictOther->mMap)
						{
							mMap.emplace(std::make_pair(it.first,it.second));
						}
					}
				}
				return *this;
			}
			virtual bool Has(const X::Value& key) override
			{
				auto it = mMap.find(key);
				return it != mMap.end();
			}
			bool Has(X::Value& key)
			{
				bool bOK = false;
				auto it = mMap.find(key);
				if (it != mMap.end())
				{
					bOK = true;
				}
				return bOK;
			}
			virtual bool Compare(X::Value& dict) override
			{
				// Verify that 'dict' is a valid Dict object.
				if (!dict.IsObject())
					return false;
				Object* pObj = dynamic_cast<Object*>(dict.GetObj());
				if (!pObj || pObj->GetType() != ObjType::Dict)
					return false;
				Dict* pOtherDict = dynamic_cast<Dict*>(pObj);
				if (!pOtherDict)
					return false;

				// Ensure both dictionaries have the same number of keys.
				// This check guarantees that if the passed dictionary has extra keys, the sizes will differ and we return false.
				if (mMap.size() != pOtherDict->mMap.size())
					return false;

				// Compare each key-value pair.
				for (const auto& pair : mMap)
				{
					// Look for the same key in the other dictionary.
					auto it = pOtherDict->mMap.find(pair.first);
					if (it == pOtherDict->mMap.end())
						return false; // Key not found in the passed dict.
					X::Value val = pair.second;
					// Compare the values using their string representations.
					// This may be replaced with a more robust comparison if needed.
					if (val.ToString() != it->second.ToString())
						return false;
				}
				return true;
			}

			bool Remove(X::Value& key)
			{
				bool bOK = false;
				auto it = mMap.find(key);
				if (it != mMap.end())
				{
					mMap.erase(it);
					bOK = true;
				}
				return bOK;
			}
			bool Get(X::Value& key, X::Value& val,
				X::LValue* lValue = nullptr)
			{
				auto it = mMap.find(key);
				if (it != mMap.end())
				{
					val = it->second;
					if (lValue)
					{
						*lValue = &it->second;
					}
				}
				return true;//always true to make caller OK
			}
			bool GetLValueToAssign(X::Value& key, X::Value& value);
			inline virtual bool Get(XRuntime* rt, XObj* pContext, 
				X::Port::vector<X::Value>& IdxAry, X::Value& val) override
			{
				//only take firs one from IdxAry
				return IdxAry.size()>0?Get(IdxAry[0], val):false;
			}
			virtual bool ToBytes(XlangRuntime* rt,XObj* pContext,X::XLangStream& stream) override
			{
				Object::ToBytes(rt,pContext,stream);
				size_t size = Size();
				stream << size;
				for (auto& it : mMap)
				{
					stream<<it.first;
					stream<<it.second;
				}
				return true;
			}
			virtual bool FromBytes(X::XLangStream& stream) override
			{
				//TODO:need to pass runtime,and calculate base class for some objects
				size_t size;
				stream >> size;
				for (size_t i = 0; i < size; i++)
				{
					X::Value key,val;
					stream >> key >>val;
					mMap[key] = val;
				}
				return true;
			}
			virtual const char* ToString(bool WithFormat = false) override
			{
				std::string strOut = "{\n";
				int cnt = (int)mMap.size();
				int i = 0;
				for (auto& it: mMap)
				{
					X::Value key = it.first;
					std::string strKey = key.ToString(WithFormat);
					X::Value Val = it.second;
					std::string strVal = Val.ToString(WithFormat);
					if (strVal.empty())
					{
						strVal = "\"\"";
					}
					strOut += "\t" + strKey+ ":"+ strVal;
					if (i < (cnt - 1))
					{
						strOut += ",\n";
					}
					else
					{
						strOut += "\n";
					}
					i++;
				}
				strOut += "}";
				return GetABIString(strOut);
			}
			virtual bool Call(XRuntime* rt, XObj* pContext,
				ARGS& params,
				KWARGS& kwParams,
				X::Value& retValue) override
			{
				return true;
			}
			FORCE_INLINE virtual bool GetAndUpdatePos(Iterator_Pos& pos, 
				std::vector<Value>& vals, bool getOnly) override
			{
				long long offset = (long long)pos;
				if (offset >= mMap.size())
				{
					return false;
				}
				auto it = mMap.begin();
				std::advance(it, offset);
				bool retVal = false;
				vals.push_back(it->first);
				vals.push_back(it->second);
				vals.push_back(offset);
				if (!getOnly)
				{
					pos = Iterator_Pos(offset + 1);
				}
				return true;
			}
			virtual List* FlatPack(XlangRuntime* rt, XObj* pContext,
				std::vector<std::string>& IdList, int id_offset,
				long long startIndex, long long count) override;
			virtual X::Value UpdateItemValue(XlangRuntime* rt, XObj* pContext,
				std::vector<std::string>& IdList, int id_offset,
				std::string itemName, X::Value& val) override;
			virtual void Enum(XDict::Dict_Enum proc) override
			{
				AutoLock autoLock(m_lock);

				for (auto& it : mMap)
				{
					X::Value key(it.first);
					X::Value val(it.second);
					proc(key, val);
				}
			}
			virtual bool Iterate(X::XRuntime* rt, XObj* pContext,
				IterateProc proc, ARGS& params, KWARGS& kwParams,
				X::Value& retValue) override
			{
				AutoLock autoLock(m_lock);
				
				for (auto& it: mMap)
				{
					X::Value key(it.first);
					X::Value val(it.second);
					proc(rt, pContext,key,val, params, kwParams);
				}
				return true;
			}
		};
	}
}
