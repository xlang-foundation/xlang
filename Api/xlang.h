#ifndef _X_LANG_H_
#define _X_LANG_H_

#include <string>
#include "value.h"

namespace X
{
	enum class ObjType
	{
		Base,
		Str,
		Binary,
		Expr,
		Function,
		MetaFunction,
		XClassObject,
		FuncCalls,
		Package,
		Future,
		List,
		Dict,
		TableRow,
		Table,
		PyProxyObject
	};
	class XObj
	{
	public:
		virtual int IncRef() = 0;
		virtual int DecRef() = 0;
		virtual ObjType GetObjType() = 0;
		virtual std::string GetTypeString() = 0;
		virtual long long Size() = 0;
		virtual size_t Hash() = 0;
		virtual std::string ToString(bool WithFormat = false)=0;

		virtual XObj& operator +=(Value& r)
		{
			return *this;
		}
		virtual XObj& operator -=(Value& r)
		{
			return *this;
		}
		virtual XObj& operator *=(Value& r)
		{
			return *this;
		}
		virtual XObj& operator /=(Value& r)
		{
			return *this;
		}
		virtual int cmp(Value* r)
		{
			return 0;
		}

	};
	class XFunc:
		virtual public XObj
	{

	};
}

#endif