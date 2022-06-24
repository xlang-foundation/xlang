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
			void Set(AST::Value& key, AST::Value& val)
			{
				mMap.emplace(std::make_pair(key, val));
			}
			virtual Dict& operator +=(AST::Value& r)
			{
				if (r.IsObject())
				{
					Object* pObj = (Object*)r.GetObject();
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
			virtual std::string ToString() override
			{
				std::string strOut = "{\n";
				for (auto& it: mMap)
				{
					AST::Value key = it.first;
					std::string strKey = key.ToString();
					AST::Value Val = it.second;
					std::string strVal = Val.ToString();
					strOut += "\t" + strKey+ ":"+ strVal +"\n";
				}
				strOut += "}";
				return strOut;
			}
			virtual bool Call(Runtime* rt, std::vector<AST::Value>& params,
				std::unordered_map<std::string, AST::Value>& kwParams,
				AST::Value& retValue) override
			{
				return true;
			}
		};
	}
}
