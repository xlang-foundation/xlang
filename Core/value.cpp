#include "value.h"
#include "object.h"
#include "str.h"
namespace X {namespace AST {
	
	ARITH_OP_IMPL(+= )
	ARITH_OP_IMPL(-= )
	ARITH_OP_IMPL(*= )
	ARITH_OP_IMPL(/= )
	COMPARE_OP_IMPL(== )
	COMPARE_OP_IMPL(!= )
	COMPARE_OP_IMPL(> )
	COMPARE_OP_IMPL(< )
	COMPARE_OP_IMPL(>= )
	COMPARE_OP_IMPL(<= )

	bool Value::Clone()
	{
		return true;
		if (t == ValueType::Object)
		{
			if (x.obj && x.obj->GetType() == Data::Type::Str)
			{
				Data::Str* pOldObj = (Data::Str*)x.obj;
				Data::Str* pStrObj = new Data::Str(pOldObj->ToString());
				pStrObj->AddRef();//for this Value
				x.obj->Release();
				x.obj = pStrObj;
			}
		}
		return true;
	}
	void Value::AssignObject(Data::Object* p)
	{
		if (p)
		{
			p->AddRef();
		}
		x.obj = p;
	}
	bool Value::ChangeToStrObject()
	{
		if (t == ValueType::Object)
		{
			return true;
		}
		else if (t == ValueType::Str)
		{
			Data::Str* pStrObj = new Data::Str((const char*)x.str,(int)flags);
			pStrObj->AddRef();//for this Value
			x.obj = pStrObj;
		}
		else
		{
			x.obj = nullptr;
		}
		t = ValueType::Object;
		return true;
	}
	void Value::ReleaseObject(Data::Object* p)
	{
		if (p)
		{
			p->Release();
		}
	}
	size_t Value::Hash()
	{
		size_t h = 0;
		switch (t)
		{
		case ValueType::Int64:
			h = std::hash<long long>{}(x.l);
			break;
		case ValueType::Double:
			h = std::hash<double>{}(x.d);
			break;
		case ValueType::Str:
			h = std::hash<std::string>{}(std::string((char*)x.str,
				(size_t)flags));
			break;
		case ValueType::Object:
			h = ((Data::Object*)x.obj)->Hash();
			break;
		default:
			break;
		}
		return h;
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