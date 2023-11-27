#pragma once
#include "value.h"
#include "lvalue.h"

namespace X
{
	namespace AST
	{
		class DynamicScope
		{
		public:
			virtual int AddOrGet(const char* name, bool bGetOnly) = 0;
			virtual bool Get(int idx, X::Value& v, X::LValue* lValue = nullptr) = 0;
			virtual bool Set(int idx, X::Value& v) = 0;
		};
	}
}