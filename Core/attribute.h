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
				kwargs.resize(m_attrs.size());
				for (auto& it : m_attrs)
				{
					kwargs.Add(it.first.c_str(), it.second, true);
				}
			}
			void Set(std::string name,X::Value& v)
			{
				AutoLock autoLock(m_lock);
				m_attrs[name] = v;
			}
			X::Value Get(std::string name)
			{
				X::Value retVal;
				AutoLock autoLock(m_lock);
				auto it = m_attrs.find(name);
				if (it != m_attrs.end())
				{
					retVal = it->second;
				}
				return retVal;
			}
			void Delete(std::string name)
			{
				AutoLock autoLock(m_lock);
				auto it = m_attrs.find(name);
				if (it != m_attrs.end())
				{
					m_attrs.erase(it);
				}
			}
		};
	}
}