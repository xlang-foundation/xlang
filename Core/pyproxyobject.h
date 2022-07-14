#pragma once
#include "object.h"

namespace X
{
	namespace Data
	{
		//wrap for Python PyObject through PyEng::Object
		class PyProxyObject :
			public Object
		{
		public:
			virtual bool Call(Runtime* rt, ARGS& params,
				KWARGS& kwParams, AST::Value& retValue) override;
		}
	}
}