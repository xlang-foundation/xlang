#pragma once
#include "value.h"

namespace X
{
	class ClassBase
	{
	public:
		virtual int GetIndex(std::string& name) = 0;
		virtual bool Set(int idx, AST::Value& v) = 0;
		virtual bool Get(int idx, AST::Value& v) = 0;
	};
}
