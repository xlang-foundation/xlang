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
			virtual std::string ToString(bool WithFormat = false)
			{
				std::string strRet= m_func->GetNameString();
				if (strRet.empty())
				{
					char v[1000];
					snprintf(v, sizeof(v), "f[%llx]",
						(unsigned long long)this);
					strRet = v;
				}
				if (WithFormat)
				{
					strRet = "\"" + strRet + "\"";
				}
				return strRet;
			}
			AST::Func* GetFunc() { return m_func; }
			virtual bool Call(Runtime* rt, ARGS& params,
				KWARGS& kwParams,
				AST::Value& retValue);
		};

	}
}