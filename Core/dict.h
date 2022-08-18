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
			public Object
		{
		protected:
			std::unordered_map<X::Value,
				X::Value, ObjectHashFunction> mMap;
		public:
			Dict()
			{
				m_t = ObjType::Dict;
			}
			~Dict()
			{
				mMap.clear();
			}
			inline virtual long long Size() override
			{
				return mMap.size();
			}
			void Set(X::Value& key, X::Value& val)
			{
				mMap.emplace(std::make_pair(key, val));
			}
			void Set(const char* key, X::Value val)
			{
				mMap.emplace(std::make_pair(X::Value(key), val));
			}
			virtual Dict& operator +=(X::Value& r)
			{
				if (r.IsObject())
				{
					Object* pObj = dynamic_cast<Object*>(r.GetObj());
					if (pObj->GetType() == ObjType::Dict)
					{
						Dict* pDictOther = (Dict*)pObj;
						for (auto& it : pDictOther->mMap)
						{
							mMap.emplace(std::make_pair(it.first,it.second));
						}
					}
				}
				return *this;
			}
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
				return bOK;
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
			virtual bool Call(Runtime* rt, ARGS& params,
				KWARGS& kwParams,
				X::Value& retValue) override
			{
				return true;
			}
			virtual List* FlatPack(Runtime* rt, 
				long long startIndex, long long count) override;

		};
	}
}
