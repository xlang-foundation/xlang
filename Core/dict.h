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
		protected:
			std::vector<AST::Scope*> m_bases;
			std::unordered_map<X::Value,
				X::Value, ObjectHashFunction> mMap;
		public:
			static void cleanup();
			Dict();
			~Dict()
			{
				AutoLock(m_lock);
				mMap.clear();
				m_bases.clear();
			}
			inline virtual long long Size() override
			{
				return mMap.size();
			}
			virtual void Set(X::Value& key, X::Value& val) override
			{
				mMap.emplace(std::make_pair(key, val));
			}
			void SetKV(X::Value& key,const X::Value& val)
			{
				mMap.emplace(std::make_pair(key, val));
			}
			virtual void GetBaseScopes(std::vector<AST::Scope*>& bases) override
			{
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
			void HookLValue(X::Value& key,X::LValue* lValue);
			bool Get(X::Value& key, X::Value& val,
				X::LValue* lValue = nullptr)
			{
				bool bOK = false;
				auto it = mMap.find(key);
				if (it != mMap.end())
				{
					val = it->second;
					if (lValue) *lValue = &it->second;
					bOK = true;
				}
				else
				{
					if (lValue)
					{
						HookLValue(key, lValue);
					}
				}
				return bOK;
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
			virtual std::string ToString(bool WithFormat = false) override
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
				return strOut;
			}
			virtual bool Call(XRuntime* rt, XObj* pContext,
				ARGS& params,
				KWARGS& kwParams,
				X::Value& retValue) override
			{
				return true;
			}
			virtual List* FlatPack(XlangRuntime* rt, XObj* pContext,
				std::vector<std::string>& IdList, int id_offset,
				long long startIndex, long long count) override;
			virtual X::Value UpdateItemValue(XlangRuntime* rt, XObj* pContext,
				std::vector<std::string>& IdList, int id_offset,
				std::string itemName, X::Value& val) override;
			virtual bool Iterate(X::XRuntime* rt, XObj* pContext,
				IterateProc proc, ARGS& params, KWARGS& kwParams,
				X::Value& retValue) override
			{
				AutoLock(m_lock);
				
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
