#include "value.h"
#include "object.h"
namespace X {namespace AST {
	
	ARITH_OP_IMPL(+= )
	ARITH_OP_IMPL(-= )
	ARITH_OP_IMPL(*= )
	ARITH_OP_IMPL(/= )

	void Value::AssignObject(Data::Object* p)
	{
		if (p)
		{
			p->AddRef();
		}
		x.obj = p;
	}
	void Value::ReleaseObject(Data::Object* p)
	{
		if (p)
		{
			p->Release();
		}
	}
	std::string Value::ToString()
	{
		std::string str;
		switch (t)
		{
		case ValueType::None:
			break;
		case ValueType::Int64:
		{
			if (flags == BOOL_FLAG)
			{
				str = (x.l == 1) ? "True" : "False";
			}
			else
			{
				char v[1000];
				snprintf(v, sizeof(v), "%lld", x.l);
				str = v;
			}
		}
			break;
		case ValueType::Double:
		{
			char v[1000];
			snprintf(v, sizeof(v), "%f", x.d);
			str = v;
		}
		break;
		case ValueType::Object:
		if(x.obj)
		{
			str = x.obj->ToString();
		}
			break;
		case ValueType::Str:
			str = std::string((char*)x.str, flags);
			break;
		default:
			break;
		}
		return str;
	}
}
}