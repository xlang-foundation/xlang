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
			size_t operator()(const AST::Value& key) const
			{
				AST::Value v =key;
				return v.Hash();
			}
		};
		class Dict :
			public Object
		{
		protected:
			std::unordered_map<AST::Value,
				AST::Value, ObjectHashFunction> mMap;
		public:
			Dict()
			{
				m_t = Type::Dict;
			}
			~Dict()
			{
				mMap.clear();
			}
			void Set(AST::Value& key, AST::Value& val)
			{
				mMap.emplace(std::make_pair(key, val));
			}
			void Set(const char* key, AST::Value val)
			{
				mMap.emplace(std::make_pair(AST::Value(key), val));
			}
			virtual Dict& operator +=(AST::Value& r)
			{
				if (r.IsObject())
				{
					Object* pObj = (Object*)r.GetObj();
					if (pObj->GetType() == Type::Dict)
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
			bool Get(AST::Value& key, AST::Value& val,
				AST::LValue* lValue = nullptr)
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
				int cnt = mMap.size();
				int i = 0;
				for (auto& it: mMap)
				{
					AST::Value key = it.first;
					std::string strKey = key.ToString(WithFormat);
					AST::Value Val = it.second;
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
				AST::Value& retValue) override
			{
				return true;
			}
		};
	}
}
