/*
Copyright (C) 2025 The XLang Foundation
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

#include "XLangStream.h"
#include <stdexcept>
#include "object.h"
#include "str.h"
#include "list.h"
#include "dict.h"
#include "set.h"
#include "bin.h"
#include "function.h"
#include "remote_object.h"
#include "moduleobject.h"
#include "struct.h"
#include "deferred_object.h"
#include "funclist.h"
#include "xproxy.h"
#include "tensor.h"
#include "complex.h"
#include "range.h"
#include "error_obj.h"

namespace X
{
	// Threshold for passing large objects by reference
	static const long long MARSHAL_SIZE_THRESHOLD = 1000;

	// Helper function: Convert XObj pointer to ROBJ_ID
	static X::ROBJ_ID ConvertXObjToId(X::XObj* obj)
	{
		if (obj == nullptr)
		{
			return X::ROBJ_ID{ 0, nullptr };
		}
		// For remote object, return its existing ID
		if (obj->GetType() == X::ObjType::RemoteObject)
		{
			auto* pRemoteObj = dynamic_cast<X::RemoteObject*>(obj);
			return pRemoteObj->GetObjId();
		}
		else
		{
			// For local objects, increment ref and create ID with current PID
			obj->IncRef();
			auto pid = GetPID();
			return X::ROBJ_ID{ pid, obj };
		}
	}

	// Helper function: Convert ROBJ_ID back to XObj pointer
	static X::XObj* ConvertIdToXObj(X::ROBJ_ID id, X::XProxy* proxy)
	{
		X::XObj* pObjRet = nullptr;
		auto pid = GetPID();
		if (id.objId == nullptr)
		{
			return nullptr;
		}
		if (pid != id.pid)
		{
			// Different process - create a RemoteObject to represent it
			auto* pRemoteObj = new X::RemoteObject(proxy);
			pRemoteObj->SetObjID(id.pid, id.objId);
			pRemoteObj->IncRef();
			pObjRet = dynamic_cast<X::XObj*>(pRemoteObj);
		}
		else
		{
			// Same process - direct pointer cast
			pObjRet = (X::XObj*)id.objId;
			if (pObjRet->GetType() == X::ObjType::RemoteObject)
			{
				auto* pRemoteClientObj = dynamic_cast<X::RemoteObject*>(pObjRet);
				pRemoteClientObj->SetProxy(proxy);
			}
		}
		return pObjRet;
	}

	// Helper function: Check if an object type should always be passed by reference (ROBJ_ID)
	static bool ShouldAlwaysPassByReference(X::ObjType objType)
	{
		switch (objType)
		{
			// Code/execution types - always pass by reference
		case X::ObjType::Function:
		case X::ObjType::MetaFunction:
		case X::ObjType::Expr:
		case X::ObjType::ConstExpr:
		case X::ObjType::FuncCalls:
		case X::ObjType::ModuleObject:
		case X::ObjType::XClassObject:
		case X::ObjType::DeferredObject:
		case X::ObjType::Future:
		case X::ObjType::TaskPool:
		case X::ObjType::Iterator:
		case X::ObjType::Prop:
		case X::ObjType::ObjectEvent:
		case X::ObjType::Type:
		case X::ObjType::TensorExpression:
		case X::ObjType::TensorOperator:
		case X::ObjType::TensorGraph:
		case X::ObjType::PyProxyObject:
		case X::ObjType::InternalAssign:
		case X::ObjType::Ref:
		case X::ObjType::StructField:
		case X::ObjType::RemoteObject:
			return true;

		default:
			return false;
		}
	}

	// Helper function: Check if a type is a container that needs recursive marshaling
	static bool IsContainerType(X::ObjType objType)
	{
		switch (objType)
		{
		case X::ObjType::List:
		case X::ObjType::Dict:
		case X::ObjType::Set:
		case X::ObjType::Struct:
		case X::ObjType::Table:
		case X::ObjType::TableRow:
			return true;
		default:
			return false;
		}
	}

	// Helper function: Check if a type can be serialized directly (no recursive check needed)
	static bool IsDirectSerializeType(X::ObjType objType)
	{
		switch (objType)
		{
		case X::ObjType::Str:
		case X::ObjType::Binary:
		case X::ObjType::Complex:
		case X::ObjType::Range:
		case X::ObjType::Error:
		case X::ObjType::Tensor:
			return true;
		default:
			return false;
		}
	}

	bool XLangStream::MarshalToBytes(X::Value& v)
	{
		auto t = v.GetType();
		(*this) << (char)t;

		switch (t)
		{
		case X::ValueType::Invalid:
		case X::ValueType::None:
			// No additional data needed
			break;

		case X::ValueType::Int64:
			(*this) << v.GetLongLong();
			break;

		case X::ValueType::Double:
			(*this) << v.GetDouble();
			break;

		case X::ValueType::Str:
			(*this) << v.ToString();
			break;

		case X::ValueType::Object:
		{
			X::XObj* pObj = v.GetObj();
			if (pObj == nullptr)
			{
				// Write a marker for null object
				(*this) << (char)X::ObjType::Base;
				(*this) << (bool)true; // isNull flag
				break;
			}

			X::ObjType objType = pObj->GetType();
			(*this) << (char)objType;
			(*this) << (bool)false; // isNull flag

			// Check if should always pass by reference
			if (objType == X::ObjType::Package)
			{
				auto* pPack = dynamic_cast<X::XPackage*>(pObj);
				bool passByRef = !(pPack && pPack->IsValuePackage());
				(*this) << passByRef;

				if (passByRef)
				{
					X::ROBJ_ID retId = ConvertXObjToId(pObj);
					(*this) << retId.pid;
					(*this) << (unsigned long long)retId.objId;
				}
				else
				{
					// Serialize by value
					X::Data::Object* pDataObj = dynamic_cast<X::Data::Object*>(pObj);
					if (pDataObj)
					{
						pDataObj->ToBytes(m_scope_space->RT(), m_scope_space->Context(), *this);
					}
				}
			}
			else if (ShouldAlwaysPassByReference(objType))
			{
				X::ROBJ_ID retId = ConvertXObjToId(pObj);
				(*this) << retId.pid;
				(*this) << (unsigned long long)retId.objId;
			}
			// Container types - need recursive marshaling
			else if (IsContainerType(objType))
			{
				// Check size for large containers
				long long size = pObj->Size();
				bool passByRef = (size > MARSHAL_SIZE_THRESHOLD);
				(*this) << passByRef;

				if (passByRef)
				{
					X::ROBJ_ID retId = ConvertXObjToId(pObj);
					(*this) << retId.pid;
					(*this) << (unsigned long long)retId.objId;
				}
				else
				{
					// Serialize container with recursive marshaling of elements
					switch (objType)
					{
					case X::ObjType::List:
					{
						X::Data::List* pList = dynamic_cast<X::Data::List*>(pObj);
						long long count = pList->Size();
						(*this) << count;
						for (long long i = 0; i < count; i++)
						{
							X::Value item = pList->Get(i);
							MarshalToBytes(item); // Recursive call
						}
					}
					break;

					case X::ObjType::Dict:
					{
						X::Data::Dict* pDict = dynamic_cast<X::Data::Dict*>(pObj);
						long long count = pDict->Size();
						(*this) << count;
						pDict->Enum([this](X::Value& key, X::Value& val)
							{
								MarshalToBytes(const_cast<X::Value&>(key)); // Recursive call
								MarshalToBytes(const_cast<X::Value&>(val)); // Recursive call
							});
					}
					break;

					case X::ObjType::Set:
					{
						X::Data::XlangSet* pSet = dynamic_cast<X::Data::XlangSet*>(pObj);
						long long count = pSet->Size();
						(*this) << count;
						for (long long i = 0; i < count; i++)
						{
							X::Value item;
							pSet->Get(i, item);
							MarshalToBytes(const_cast<X::Value&>(item));
						}
					}
					break;

					case X::ObjType::Struct:
					{
						X::Data::XlangStruct* pStruct = dynamic_cast<X::Data::XlangStruct*>(pObj);
						long long count = pStruct->Size();
						(*this) << count;
						for (long long i = 0; i < count; i++)
						{
							X::Value item;
							pStruct->GetIndexValue(i, item);
							MarshalToBytes(item); // Recursive call
						}
					}
					break;

					case X::ObjType::Table:
					case X::ObjType::TableRow:
					{
						// For Table types, use existing ToBytes for now
						X::Data::Object* pDataObj = dynamic_cast<X::Data::Object*>(pObj);
						pDataObj->ToBytes(m_scope_space->RT(), m_scope_space->Context(), *this);
					}
					break;

					default:
						break;
					}
				}
			}
			else
			{
				X::Data::Object* pDataObj = dynamic_cast<X::Data::Object*>(pObj);
				if (pDataObj)
				{
					pDataObj->ToBytes(m_scope_space->RT(), m_scope_space->Context(), *this);
				}
			}
		}
		break;

		default:
			break;
		}

		return true;
	}

	bool XLangStream::MarshalFromBytes(X::Value& v, X::XProxy* proxy)
	{
		char ch;
		(*this) >> ch;
		X::ValueType t = (X::ValueType)ch;

		switch (t)
		{
		case X::ValueType::Invalid:
			v.SetType(X::ValueType::Invalid);
			break;

		case X::ValueType::None:
			v.SetType(X::ValueType::None);
			break;

		case X::ValueType::Int64:
		{
			long long l;
			(*this) >> l;
			v = X::Value(l);
		}
		break;

		case X::ValueType::Double:
		{
			double d;
			(*this) >> d;
			v = X::Value(d);
		}
		break;

		case X::ValueType::Str:
		{
			std::string s;
			(*this) >> s;
			v.SetString(s);
		}
		break;

		case X::ValueType::Object:
		{
			char objTypeCh;
			(*this) >> objTypeCh;
			X::ObjType objType = (X::ObjType)objTypeCh;

			bool isNull;
			(*this) >> isNull;
			if (isNull)
			{
				v = X::Value();
				break;
			}

			// Check if should always pass by reference
			if (objType == X::ObjType::Package)
			{
				bool passByRef;
				(*this) >> passByRef;

				if (passByRef)
				{
					X::ROBJ_ID retId;
					(*this) >> retId.pid;
					unsigned long long objIdVal;
					(*this) >> objIdVal;
					retId.objId = (void*)objIdVal;

					X::XObj* pObj = ConvertIdToXObj(retId, proxy);
					if (pObj != nullptr)
					{
						auto pid = GetPID();
						if (pid == retId.pid)
						{
							v = X::Value(pObj, true);
						}
						else
						{
							v = X::Value(pObj, false);
						}
					}
					else
					{
						v = X::Value();
					}
				}
				else
				{
					X::Data::Object* pObjToRestore = nullptr;
					unsigned long long embedId;
					(*this) >> embedId;
					bool bRefPackObj = false;
					(*this) >> bRefPackObj;
					if (bRefPackObj)
					{
						pObjToRestore = (X::Data::Object*)m_scope_space->Query(embedId);
						if (pObjToRestore) pObjToRestore->IncRef();
					}
					else
					{
						std::string strPackUri;
						(*this) >> strPackUri;
						X::Value varPackCreate = g_pXHost->CreatePackageWithUri(strPackUri.c_str(), this);
						if (varPackCreate.IsObject())
						{
							pObjToRestore = dynamic_cast<X::Data::Object*>(varPackCreate.GetObj());
							m_scope_space->Add(embedId, (void*)pObjToRestore);
							if (pObjToRestore->GetType() != X::ObjType::Package)
							{
								long long skip_size = 0;
								(*this) >> skip_size;
								if (skip_size) Skip(skip_size);
							}
							pObjToRestore->IncRef();
						}
					}
					
					if (pObjToRestore)
					{
						if (pObjToRestore->GetType() == X::ObjType::Package && !bRefPackObj)
						{
							pObjToRestore->FromBytes(*this);
						}
						v = X::Value(dynamic_cast<XObj*>(pObjToRestore), false);
					}
					else
					{
						v = X::Value();
					}
				}
			}
			else if (ShouldAlwaysPassByReference(objType))
			{
				X::ROBJ_ID retId;
				(*this) >> retId.pid;
				unsigned long long objIdVal;
				(*this) >> objIdVal;
				retId.objId = (void*)objIdVal;

				X::XObj* pObj = ConvertIdToXObj(retId, proxy);
				if (pObj != nullptr)
				{
					auto pid = GetPID();
					if (pid == retId.pid)
					{
						v = X::Value(pObj, true);
					}
					else
					{
						v = X::Value(pObj, false);
					}
				}
				else
				{
					v = X::Value();
				}
			}
			// Container types
			else if (IsContainerType(objType))
			{
				bool passByRef;
				(*this) >> passByRef;

				if (passByRef)
				{
					X::ROBJ_ID retId;
					(*this) >> retId.pid;
					unsigned long long objIdVal;
					(*this) >> objIdVal;
					retId.objId = (void*)objIdVal;

					X::XObj* pObj = ConvertIdToXObj(retId, proxy);
					if (pObj != nullptr)
					{
						auto pid = GetPID();
						if (pid == retId.pid)
						{
							v = X::Value(pObj, true);
						}
						else
						{
							v = X::Value(pObj, false);
						}
					}
					else
					{
						v = X::Value();
					}
				}
				else
				{
					// Deserialize container with recursive unmarshaling
					switch (objType)
					{
					case X::ObjType::List:
					{
						X::Data::List* pList = new X::Data::List();
						pList->IncRef();
						long long count;
						(*this) >> count;
						for (long long i = 0; i < count; i++)
						{
							X::Value item;
							MarshalFromBytes(item, proxy); // Recursive call
							pList->Add(nullptr, item);
						}
						v = X::Value(dynamic_cast<X::XObj*>(pList), false);
					}
					break;

					case X::ObjType::Dict:
					{
						X::Data::Dict* pDict = new X::Data::Dict();
						pDict->IncRef();
						long long count;
						(*this) >> count;
						for (long long i = 0; i < count; i++)
						{
							X::Value key, val;
							MarshalFromBytes(key, proxy); // Recursive call
							MarshalFromBytes(val, proxy); // Recursive call
							pDict->Set(key, val);
						}
						v = X::Value(dynamic_cast<X::XObj*>(pDict), false);
					}
					break;

					case X::ObjType::Set:
					{
						X::Data::XlangSet* pSet = new X::Data::XlangSet();
						pSet->IncRef();
						long long count;
						(*this) >> count;
						for (long long i = 0; i < count; i++)
						{
							X::Value item;
							MarshalFromBytes(item, proxy); // Recursive call
							pSet->Add(item);
						}
						v = X::Value(dynamic_cast<X::XObj*>(pSet), false);
					}
					break;

					case X::ObjType::Struct:
					{
						v = X::Value();
					}
					break;

					case X::ObjType::Table:
					case X::ObjType::TableRow:
					{
						// Use existing FromBytes - may need enhancement
						v = X::Value();
					}
					break;

					default:
						v = X::Value();
						break;
					}
				}
			}
			else
			{
				// Use existing FromBytes deserialization
				X::Data::Object* pObjToRestore = nullptr;
				switch (objType)
				{
				case X::ObjType::Str:
					pObjToRestore = dynamic_cast<X::Data::Object*>(new X::Data::Str());
					break;
				case X::ObjType::Binary:
					pObjToRestore = dynamic_cast<X::Data::Object*>(new X::Data::Binary(nullptr, 0, true));
					break;
				case X::ObjType::Complex:
					pObjToRestore = dynamic_cast<X::Data::Object*>(new X::Data::Complex());
					break;
				case X::ObjType::Range:
					pObjToRestore = dynamic_cast<X::Data::Object*>(new X::Data::Range());
					break;
				case X::ObjType::Error:
					pObjToRestore = dynamic_cast<X::Data::Object*>(new X::Data::Error());
					break;
				case X::ObjType::Tensor:
					pObjToRestore = dynamic_cast<X::Data::Object*>(new X::Data::Tensor());
					break;
				default:
					break;
				}

				if (pObjToRestore)
				{
					pObjToRestore->IncRef();
					pObjToRestore->FromBytes(*this);
					v = X::Value(dynamic_cast<X::XObj*>(pObjToRestore), false);
				}
				else
				{
					v = X::Value();
				}

			}
		}
		break;

		default:
			v = X::Value();
			break;
		}

		return true;
	}
}