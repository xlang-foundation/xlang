#include "value.h"
#include "object.h"
namespace X {namespace AST {
	
	ARITH_OP_IMPL(+= )
	ARITH_OP_IMPL(-= )
	ARITH_OP_IMPL(*= )
	ARITH_OP_IMPL(/= )

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
		if(x.p)
		{
			Data::Object* pObj = (Data::Object*)x.p;
			str = pObj->ToString();
		}
			break;
		case ValueType::Str:
			str = std::string((char*)x.p, flags);
			break;
		default:
			break;
		}
		return str;
	}
}
}