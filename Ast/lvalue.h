#pragma once

#include "value.h"

namespace X
{
	class XObj;
	class LValue
	{
		Value val;//for some cases, the value is on stack, 
		//can't keep when out of scope
		//so add here,but it is enable when valptr is null
		Value* valptr = nullptr;
		XObj* context = nullptr;
	public:
		inline LValue(Value& v)
		{
			val = v;
			valptr = &val;
		}
		inline LValue(Value* pVal)
		{
			valptr = pVal;
		}
		inline void SetContext(XObj* p) { context = p; }
		inline XObj* GetContext() { return context; }
		inline operator bool() const
		{
			return (valptr != 0);
		}
		inline Value& operator *() const
		{
			if (valptr)
			{
				return *valptr;
			}
			else
			{
				return (Value&)val;
			}
		}
		inline Value* operator ->() const
		{
			return valptr;
		}
	};
}