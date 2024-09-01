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