#ifndef _X_LANG_H_
#define _X_LANG_H_

#include <string>
#include <vector>
#include <unordered_map>
#include "value.h"
#include "xhost.h"

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
		virtual ObjType GetType() = 0;
		virtual std::string GetTypeString() = 0;
		virtual long long Size() = 0;
		virtual size_t Hash() = 0;
		virtual std::string ToString(bool WithFormat = false)=0;
		virtual bool Call(XRuntime* rt, ARGS& params,
			KWARGS& kwParams,
			X::Value& retValue) = 0;

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
	class XPackage:
		virtual public XObj
	{
	public:
		virtual int AddMethod(const char* name) = 0;
		virtual void* GetEmbedObj() = 0;
		virtual bool Init(int varNum) = 0;
		virtual bool SetIndexValue(XRuntime* rt, void* pContext, int idx, Value& v) = 0;
	};

}

#endif