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
#include "object.h"

namespace X
{
	namespace Data
	{
		class Range :
			virtual public Object
		{
			long long m_start;
			long long m_stop;
			long long m_step;
		public:
			Range() :
				Object(),
				m_start(0), m_stop(0), m_step(1)
			{
				m_t = ObjType::Range;
			}
			Range(long long start, long long stop, long long step = 1)
				:Object(),
				m_start(start), m_stop(stop), m_step(step)
			{
				m_t = ObjType::Range;
			}
			virtual bool Call(XRuntime* rt, XObj* pContext, ARGS& params,
				KWARGS& kwParams, X::Value& retValue) override
			{
				return true;
			}
			virtual bool IsContain(X::Value& val) override
			{
				if (val.GetType() != ValueType::Int64)
				{
					return false;
				}
				long long v = val;
				return v >= m_start && v < m_stop;
			}
			FORCE_INLINE virtual bool GetAndUpdatePos(Iterator_Pos& pos,
				std::vector<Value>& vals, bool getOnly) override
			{
				long long it = (long long)pos;
				long long nPos = it;
				AutoLock autoLock(m_lock);
				if (it >= (long long)m_stop)
				{
					return false;
				}
				X::Value val0 = m_start+it;
				//add step
				it += m_step;
				if (!getOnly)
				{
					pos = (Iterator_Pos)it;
				}
				vals.push_back(val0);
				vals.push_back(X::Value(nPos));
				return true;
			}
		};
	}
}