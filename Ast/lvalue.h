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
		bool releaseValPtr = false;
		XObj* context = nullptr;
	public:
		FORCE_INLINE void SetReleaseFlag(bool b)
		{
			releaseValPtr = b;
		}
		FORCE_INLINE LValue()
		{
			valptr = nullptr;
		}
		FORCE_INLINE LValue(Value& v)
		{
			val = v;
			//todo: CHANGE TO NULL, because v maybe in stack, 
			//as temp var
			valptr = nullptr;// &val;
		}
		FORCE_INLINE LValue(Value* pVal)
		{
			valptr = pVal;
		}
		FORCE_INLINE ~LValue()
		{
			if (releaseValPtr && valptr)
			{
				delete valptr;
			}
		}
		FORCE_INLINE void SetContext(XObj* p) { context = p; }
		FORCE_INLINE XObj* GetContext() { return context; }
		FORCE_INLINE operator bool()
		{
			return (valptr != 0) || val.IsValid();
		}
		FORCE_INLINE Value& operator *() const
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
		FORCE_INLINE Value* operator ->() const
		{
			return valptr;
		}
	};
}