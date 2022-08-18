#include "value.h"
//#include "object.h"
//#include "str.h"
#include "xlang.h"
#include "utility.h"
namespace X 
{
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
		if (t == ValueType::Object)
		{
			if (x.obj && x.obj->GetObjType() == ObjType::Str)
			{
				std::string oldStr = x.obj->ToString();
				auto newObj = g_pXHost->CreateStrObj(oldStr.c_str(), x.obj->Size());
				x.obj->DecRef();
				x.obj = newObj;
			}
		}
		return true;
	}
	void Value::AssignObject(XObj* p)
	{
		if (p)
		{
			p->IncRef();
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
			x.obj = g_pXHost->CreateStrObj((const char*)x.str, (int)flags);;
		}
		else
		{
			x.obj = nullptr;
		}
		t = ValueType::Object;
		return true;
	}
	void Value::ReleaseObject(XObj* p)
	{
		if (p)
		{
			p->DecRef();
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
			h = ((XObj*)x.obj)->Hash();
			break;
		default:
			break;
		}
		return h;
	}
	std::string Value::ToString(bool WithFormat)
	{
		std::string str;
		switch (t)
		{
		case ValueType::Invalid:
			if (WithFormat) str = "\"Invalid\"";
			break;
		case ValueType::None:
			if (WithFormat) str = "\"\"";
			break;
		case ValueType::Int64:
		{
			if (flags == BOOL_FLAG)
			{
				if(WithFormat) str = (x.l == 1) ? "true" : "false";//Json likes it
				else str = (x.l == 1) ? "True" : "False";
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
			str = x.obj->ToString(WithFormat);
			if (WithFormat && x.obj->GetObjType() == ObjType::Str)
			{
				str= StringifyString(str);
			}
		}
			break;
		case ValueType::Str:
			str = std::string((char*)x.str, flags);
			if (WithFormat) str = StringifyString(str);
			break;
		default:
			break;
		}
		return str;
	}
	std::string Value::GetValueType()
	{
		std::string strType;
		switch (t)
		{
		case ValueType::Invalid:
			strType = "Invalid";
			break;
		case ValueType::None:
			strType = "None";
			break;
		case ValueType::Int64:
			strType = "Int64";
			break;
		case ValueType::Double:
			strType = "Double";
			break;
		case ValueType::Object:
		{
			auto* pObj = GetObj();
			if (pObj)
			{
				strType = pObj->GetTypeString();
			}
			else
			{
				strType = "Null";
			}
		}
		break;
		case ValueType::Str:
			strType = "Str";
			break;
		case ValueType::Value:
			strType = "Value";
			break;
		default:
			break;
		}
		return strType;
	}
}