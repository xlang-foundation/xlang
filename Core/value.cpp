#include "value.h"
namespace X {namespace AST {
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
		{
			char v[1000];
			snprintf(v, sizeof(v), "Object:0x%llx", (unsigned long long)x.p);
			str = v;
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