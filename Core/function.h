#pragma once

#include "object.h"
#include <string>
namespace X
{
	namespace Data
	{
		class Function :
			public Object
		{
		protected:
			AST::Func* m_func = nullptr;
		public:
			Function(AST::Func* p);
			~Function();
			virtual std::string ToString()
			{
				char v[1000];
				snprintf(v, sizeof(v), "Function:%s@0x%llx",
					m_func->GetNameString().c_str(), (unsigned long long)this);
				return v;
			}
			AST::Func* GetFunc() { return m_func; }
			virtual bool Call(Runtime* rt, ARGS& params,
				KWARGS& kwParams,
				AST::Value& retValue);
		};

	}
}