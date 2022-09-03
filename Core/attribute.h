#pragma once
#include <string>
#include <unordered_map>
#include "value.h"
#include "Locker.h"

namespace X 
{
	namespace Data
	{
		class AttributeBag
		{
			std::unordered_map<std::string,Value> m_attrs;
			Locker m_lock;
		public:
			AttributeBag()
			{

			}
			~AttributeBag()
			{

			}
			void CovertToDict(KWARGS& kwargs)
			{
				for (auto& it : m_attrs)
				{
					kwargs.emplace(it);
				}
			}
			void Set(std::string name,X::Value& v)
			{
				AutoLock(m_lock);
				m_attrs[name] = v;
			}
			X::Value Get(std::string name)
			{
				X::Value retVal;
				AutoLock(m_lock);
				auto it = m_attrs.find(name);
				if (it != m_attrs.end())
				{
					retVal = it->second;
				}
				return retVal;
			}
			void Delete(std::string name)
			{
				AutoLock(m_lock);
				auto it = m_attrs.find(name);
				if (it != m_attrs.end())
				{
					m_attrs.erase(it);
				}
			}
		};
	}
}