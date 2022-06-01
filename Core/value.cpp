#include "value.h"
namespace XPython {namespace AST {
	std::string Value::ToString()
	{
		std::string str;
		switch (t)
		{
		case ValueType::None:
			break;
		case ValueType::Int64:
		{
			char v[1000];
			snprintf(v, sizeof(v), "%llx",x.l);
			str = v;
		}
			break;
		case ValueType::Double:
		{
			char v[1000];
			snprintf(v, sizeof(v), "%f", x.d);
			str = v;
		}
		break;
		case ValueType::Pointer:
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