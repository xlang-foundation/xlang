/*
Copyright (C) 2024 The XLang Foundation
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "value.h"
#include "xlang.h"
#include "xhost.h"

namespace X
{
	//ARITH_OP_IMPL(-= )
	ARITH_OP_IMPL(*= )
		ARITH_OP_IMPL(/= )


	Value Value::GetObjectValue(Port::vector<X::Value>& IdxAry)
	{
		auto* rt = X::g_pXHost->GetCurrentRuntime();
		XObj* pObj = GetObj();
		X::Value retVal;
		pObj->Get(rt, pObj, IdxAry, retVal);
		return retVal;
	}
	Value Value::GetItemValue(long long idx)
	{
		XObj* pObj = GetObj();
		X::Value retVal;
		pObj->GetIndexValue(idx, retVal);
		return retVal;
	}
	Value Value::operator* (const Value& right)
	{
		Value ret;
		bool done = false;
		if (t == ValueType::Object)
		{
			X::XObj* pObj = GetObj();
			done = pObj->Multiply(right, ret);
		}
		else if (right.IsObject())
		{
			done = right.GetObj()->Multiply(*this, ret);
		}
		if (!done)
		{
			ret = *this;
			ret.Clone();
			ret *= right;
		}
		return ret;
	}
	Value Value::operator/ (const Value& right)
	{
		Value ret;
		bool done = false;
		if (t == ValueType::Object)
		{
			X::XObj* pObj = GetObj();
			done = pObj->Divide(right, ret);
		}
		else if (right.IsObject())
		{
			//for case:this Value is not an object, just right side is an object
			done = right.GetObj()->Divided(*this, ret);
		}
		if (!done)
		{
			ret = *this;
			ret.Clone();
			ret /= right;
		}
		return ret;
	}
	Value Value::AddObj(const Value& right)
	{
		Value ret;
		bool done = false;
		if (t == ValueType::Object)
		{
			X::XObj* pObj = GetObj();
			done = pObj->Add(right, ret);
		}
		else if (right.IsObject())
		{
			done = right.GetObj()->Add(*this, ret);
		}
		if (!done)
		{
			ret = *this;
			ret.Clone();
			ret += right;
		}
		return ret;
	}
	Value Value::operator- (const Value& right)
	{
		if (t == ValueType::Invalid || right.t == ValueType::Invalid)
		{
			return Value();
		}
		Value ret;
		bool done = false;
		if (t == ValueType::Object)
		{
			X::XObj* pObj = GetObj();
			if (right.IsObject())
			{
				done = pObj->Minus(right, ret);
			}
			else
			{
				done = pObj->Add(right.Negative(), ret);
			}
		}
		else if (right.IsObject())
		{
			//for case:this Value is not an object, just right side is an object
			done = right.GetObj()->Minuend(*this, ret);
		}
		if (!done)
		{
			ret = *this;
			ret.Clone();
			ret -= right;
		}
		return ret;
	}

	void Value::operator -= (const Value& v)
	{
		if (t == ValueType::Invalid || v.t == ValueType::Invalid)
		{
			return;
		}
		flags = v.flags;
		if (t == ValueType::Object)
		{
			Value v0;
			//don't change V
			v0 += v;
			(*((XObj*)x.obj)) -= v0;
		}
		else if (v.IsObject())
		{
			Value v0 = v;
			v0 -= *this;
			t = ValueType::Object;
			AssignObject(v0.GetObj());
		}
		else
		{
			switch (t)
			{
			case ValueType::Int64:
				x.l -= ValueToInt64(v);
				break;
			case ValueType::Double:
				x.d -= ValueToDouble(v);
				break;
			case ValueType::Str:
				x.str = v.x.str;
				ChangeToStrObject();
				break;
			default:
				*this -= v;
				break;
			}
		}
	}

#if 0
	template<typename toT>
	Value::operator toT* () const
	{
		if (x.obj->GetType() == ObjType::Package)
		{
			XPackage* pPack = dynamic_cast<XPackage*>(x.obj);
			return (toT*)pPack->GetEmbedObj();
		}
		else
		{
			return (toT*)x.obj;
		}
	}
#endif

	void* Value::CastObjectToPointer() const
	{
		if (x.obj->GetType() == ObjType::Package)
		{
			XPackage* pPack = dynamic_cast<XPackage*>(x.obj);
			return pPack->GetEmbedObj();
		}
		else
		{
			return x.obj;
		}
	}
	template<>
	void V<XRuntime>::Create()
	{
		SetObj(g_pXHost->CreateRuntime());
	}
	template<>
	template<>
	void V<XModule>::Create(char* xModuleFile)
	{
		g_pXHost->Import(g_pXHost->GetCurrentRuntime(), xModuleFile, nullptr, nullptr, *this);
	}
	template<>
	template<>
	void V<XModule>::Create(char* xModuleFile, char* from)
	{
		g_pXHost->Import(g_pXHost->GetCurrentRuntime(), xModuleFile, from, nullptr, *this);
	}
	template<>
	template<>
	void V<XModule>::Create(char* xModuleFile, char* from,char* thru)
	{
		g_pXHost->Import(g_pXHost->GetCurrentRuntime(), xModuleFile, from, thru, *this);
	}
	template<>
	void V<XDict>::Create()
	{
		SetObj(g_pXHost->CreateDict());
	}
	template<>
	void V<XList>::Create()
	{
		SetObj(g_pXHost->CreateList());
	}
	template<>
	void V<XTensor>::Create()
	{
		SetObj(g_pXHost->CreateTensor());
	}
	template<>
	void V<XComplex>::Create()
	{
		SetObj(g_pXHost->CreateComplex());
	}
	template<>
	template<>
	void V<XSet>::Create()
	{
		SetObj(g_pXHost->CreateSet());
	}
	template<>
	template<>
	void V<XStr>::Create(int size)
	{
		SetObj(g_pXHost->CreateStr(nullptr, size));
	}
	template<>
	template<>
	void V<XError>::Create(int code,const char* info)
	{
		SetObj(g_pXHost->CreateError(code, info));
	}
	template<>
	template<>
	void V<XStruct>::Create(char* data, int size, bool asRef)
	{
		SetObj(g_pXHost->CreateStruct(data, size, asRef));
	}
	//only need one template<>
	template<>
	void V<XStruct>::Create(void)
	{
		SetObj(g_pXHost->CreateStruct(nullptr,0,false));
	}
	template<>
	template<>
	void V<XStr>::Create(const char* s, int size)
	{
		SetObj(g_pXHost->CreateStr(s, size));
	}
	template<>
	template<>
	void V<XBin>::Create(char* s, int size, bool bOwnData)
	{
		SetObj(g_pXHost->CreateBin(s, size, bOwnData));
	}
	template<>
	template<>
	void V<XBin>::Create(char* s, unsigned long long size, bool bOwnData)
	{
		SetObj(g_pXHost->CreateBin(s, size, bOwnData));
	}
	template<>
	template<>
	void V<XPackage>::Create(void* pRealObj)
	{
		SetObj(g_pXHost->CreatePackage(pRealObj));
	}
	template<>
	template<>
	void V<XPackage>::Create(V<XPackage> package, void* pRealObj)
	{
		SetObj(g_pXHost->CreatePackageProxy(package, pRealObj));
	}
	template<>
	template<>
	void V<XEvent>::Create(const char* name)
	{
		SetObj(g_pXHost->CreateXEvent(name));
	}
	template<>
	template<>
	void V<XFunc>::Create(const char* name, U_FUNC func, X::XObj* pContext)
	{
		SetObj(g_pXHost->CreateFunction(name, func, pContext));
	}
	template<>
	template<>
	void V<XFunc>::Create(const char* name, U_FUNC func)
	{
		SetObj(g_pXHost->CreateFunction(name, func));
	}
	template<>
	template<>
	void V<XFunc>::Create(const char* name, U_FUNC_EX func, X::XObj* pContext)
	{
		SetObj(g_pXHost->CreateFunctionEx(name, func, pContext));
	}
	template<>
	template<>
	void V<XFunc>::Create(const char* name, U_FUNC_EX func)
	{
		SetObj(g_pXHost->CreateFunctionEx(name, func));
	}
	template<>
	template<>
	void V<XPackage>::Create(Runtime rt, const char* moduleName,
		const char* from, const char* thru)
	{
		Value v0;
		if (g_pXHost->Import(rt, moduleName, from, thru, v0))
		{
			auto* pObj = v0.GetObj();
			if (pObj)
			{
				pObj->IncRef();
				SetObj(pObj);
			}
		}
	}
	template<>
	template<>
	void V<XPackage>::Create(Runtime rt, const char* moduleName)
	{
		Value v0;
		if (g_pXHost->Import(rt, moduleName, nullptr, nullptr, v0))
		{
			auto* pObj = v0.GetObj();
			if (pObj)
			{
				pObj->IncRef();
				SetObj(pObj);
			}
		}
	}
	template<>
	template<>
	void V<XPackage>::Create(Runtime rt, const char* moduleName,
		const char* from)
	{
		Value v0;
		if (g_pXHost->Import(rt, moduleName, from, nullptr, v0))
		{
			auto* pObj = v0.GetObj();
			if (pObj)
			{
				pObj->IncRef();
				SetObj(pObj);
			}
		}
	}
	Value Value::ObjCall(Port::vector<X::Value>& params, Port::StringMap<X::Value>& kwParams)
	{
		auto* pObj = GetObj();
		if (pObj == nullptr)
		{
			return Value();
		}
		Value v0;
		pObj->Call(pObj->RT(), pObj->Parent(), params, kwParams, v0);
		if (v0.IsObject())
		{
			v0.GetObj()->SetContext(pObj->RT(), pObj->Parent());
		}
		return v0;
	}
	Value Value::ObjCallEx(Port::vector<X::Value>& params,
		Port::StringMap<X::Value>& kwParams, X::Value& trailer)
	{
		auto* pObj = GetObj();
		if (pObj == nullptr || pObj->RT() == nullptr)
		{
			return Value();
		}
		Value v0;
		pObj->CallEx(pObj->RT(), pObj->Parent(), params, kwParams, trailer, v0);
		if (v0.IsObject())
		{
			v0.GetObj()->SetContext(pObj->RT(), pObj->Parent());
		}
		return v0;
	}
	Value Value::ObjCall(Port::vector<X::Value>& params)
	{
		auto* pObj = GetObj();
		if (pObj == nullptr || pObj->RT() == nullptr)
		{
			return Value();
		}
		KWARGS kwargs;
		Value v0;
		pObj->Call(pObj->RT(), pObj->Parent(), params, kwargs, v0);
		if (v0.IsObject())
		{
			v0.GetObj()->SetContext(pObj->RT(), pObj->Parent());
		}
		return v0;
	}
	Value Value::QueryMember(const char* key)
	{
		if (IsObject())
		{
			return GetObj()->Member(key);
		}
		else
		{
			return Value();
		}
	}
	bool Value::Clone()
	{
		if (t == ValueType::Object)
		{
			if (x.obj)
			{
				auto* pNewObj = x.obj->Clone();
				x.obj->DecRef();
				x.obj = pNewObj;
			}
		}
		return true;
	}
	void Value::ObjectAssignAndAdd(const Value& v)
	{
		if(t == ValueType::Object && v.t != ValueType::Object)
		{
			//if this is an object, but v is not an object, then we need to
			//add v to this object
			x.obj->operator+=((Value&)v);
		}
		else if (t != ValueType::Object && v.t == ValueType::Object)
		{
			//if this is not an object, but v is an object, then we need to
			//add this to v object
			*v.x.obj += *this;
		}
		else
		{
			x.obj->operator+=((Value&)v);
		}
	}
	void Value::AssignObject(XObj* p, bool bAddRef)
	{
		if (p == nullptr)
		{
			x.obj = nullptr;
			return;
		}
		if (bAddRef)
		{
			//for string object, need to clone a new one to assign
			//for other object, just assign the pointer
			if (p->GetType() == ObjType::Str)
			{
				p = p->Clone();
			}
			else
			{
				p->IncRef();
			}
		}
		x.obj = p;
	}
	void Value::SetObject(XObj* p)
	{
		if (p != nullptr)
		{
			p->IncRef();
		}
		x.obj = p;
	}
	void Value::SetString(std::string& s)
	{
		t = ValueType::Object;
		x.obj = dynamic_cast<XObj*>(g_pXHost->CreateStr(s.c_str(), (int)s.size()));
	}
	void Value::SetString(std::string&& s)
	{
		t = ValueType::Object;
		x.obj = g_pXHost->CreateStr(s.c_str(), (int)s.size());
	}
	Value::Value(std::string& s)
	{
		SetString(s);
	}
	Value::Value(std::string&& s)
	{
		SetString(s);
	}

	int Value::obj_cmp(Value* r) const
	{
		//if r is const str and this is a string object 
		if (t == X::ValueType::Object && GetObj()->GetType() == X::ObjType::Str
			&& r->t == X::ValueType::Str)
		{
			return x.obj->cmp(r);
		}
		if (t != r->t)
		{
			return 1;
		}
		if (x.obj->GetType() != r->x.obj->GetType())
		{
			return 1;
		}
		return x.obj->cmp(r);
	}
	bool Value::ChangeToStrObject()
	{
		if (t == ValueType::Object)
		{
			return true;
		}
		else if (t == ValueType::Str)
		{
			x.obj = g_pXHost->CreateStr((const char*)x.str, (int)flags);
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
			ValueSubType last4Digit = (ValueSubType)(flags & 0xF);
			switch (last4Digit)
			{
				case ValueSubType::BOOL:
				{
					if (WithFormat) str = (x.l == 1) ? "true" : "false";//Json likes it
					else str = (x.l == 1) ? "True" : "False";
				}
				break;
				case ValueSubType::CHAR:
					str = (char)x.l;
				break;
				case ValueSubType::UINT64:
				{
					char v[1000];
					snprintf(v, sizeof(v), "%llu", (unsigned long long)x.l);
					str = v;
				}
				break;
				default:
				{
					char v[1000];
					snprintf(v, sizeof(v), "%lld", x.l);
					str = v;
				}
			}//end switch (last4Digit)
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
			if (x.obj)
			{
				auto str_abi = x.obj->ToString(WithFormat);
				str = str_abi;
				X::g_pXHost->ReleaseString(str_abi);
				if (WithFormat && x.obj->GetType() == ObjType::Str)
				{
					const char* pNewStr = g_pXHost->StringifyString(str.c_str());
					if (pNewStr)
					{
						str = pNewStr;
						g_pXHost->ReleaseString(pNewStr);
					}
				}
			}
			break;
		case ValueType::Str:
			str = std::string((char*)x.str, flags);
			if (WithFormat)
			{
				const char* pNewStr = g_pXHost->StringifyString(str.c_str());
				if (pNewStr)
				{
					str = pNewStr;
					g_pXHost->ReleaseString(pNewStr);
				}
			}
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
				const char* retStrType = pObj->GetTypeString();
				if (retStrType)
				{
					strType = std::string(retStrType);
					g_pXHost->ReleaseString(retStrType);
				}
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
	bool Value::CallAssignIfObjectSupport(const Value& v)
	{
		if (x.obj->SupportAssign())
		{
			x.obj->Assign(v);
			return true;
		}
		return false;
	}
	long long Value::Size()
	{
		long long  sizeRet = 0;
		switch (t)
		{
		case ValueType::Int64:
			sizeRet = 1;
			break;
		case ValueType::Double:
			sizeRet = 1;
			break;
		case ValueType::Object:
			sizeRet = (x.obj != nullptr ? x.obj->Size() : 0);
			break;
		case ValueType::Str:
			sizeRet = flags;
		default:
			break;
		}
		return sizeRet;
	}
	bool Value::IsList() const
	{
		return (t == ValueType::Object)
			&& (x.obj != nullptr && x.obj->GetType() == ObjType::List);
	}
	bool Value::IsDict() const
	{
		return (t == ValueType::Object)
			&& (x.obj != nullptr && x.obj->GetType() == ObjType::Dict);
	}
	bool Value::IsString() const
	{ 
		if (t == ValueType::Str)
		{
			return true;
		}
		else
		{
			return (t == ValueType::Object)
				&& (x.obj != nullptr && x.obj->GetType() == ObjType::Str);
		}
	}
	bool Value::ToBytes(XLStream* pStream)
	{
		return g_pXHost->ConvertToBytes(*this, pStream);
	}
	bool Value::FromBytes(XLStream* pStream)
	{
		return g_pXHost->ConvertFromBytes(*this, pStream);
	}
	Value Value::getattr(const char* attrName) const
	{
		return g_pXHost->GetAttr(*this, attrName);
	}
	void Value::setattr(const char* attrName, X::Value& attrVal) const
	{
		g_pXHost->SetAttr(*this, attrName, attrVal);
	}
}