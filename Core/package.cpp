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

#include "package.h"
#include "list.h"
#include "dict.h"
#include "prop.h"
#include "event.h"
#include "port.h"
#include "utility.h"
#include "Hosting.h"
#include "xpackage.h"

namespace X
{
	namespace AST
	{
		std::string GetMemberType(PackageMemberType type)
		{
			std::string valType;
			switch (type)
			{
			case PackageMemberType::Func:
			case PackageMemberType::FuncEx:
				valType = "Function";
				break;
			case PackageMemberType::Class:
			case PackageMemberType::ClassInstance:
				valType = "Class";
				break;
			case PackageMemberType::Prop:
				valType = "Prop";
				break;
			case PackageMemberType::ObjectEvent:
				valType = "Event";
				break;
			default:
				break;
			}
			return valType;
		}
		X::Value Package::UpdateItemValue(XlangRuntime* rt, XObj* pContext,
			std::vector<std::string>& IdList, int id_offset,
			std::string itemName, X::Value& val)
		{
			X::Value retVal = val;
			AutoLock autoLock(m_lock);
			if (id_offset < IdList.size())
			{
				auto& strId = IdList[id_offset++];
				X::Value item;
				SCOPE_FAST_CALL_AddOrGet0(idx,m_pMyScope,strId, true);
				if (idx >= 0)
				{
					GetIndexValue(idx, item);
					if (item.IsObject())
					{
						Object* pChildObj = dynamic_cast<Object*>(item.GetObj());
						if (pChildObj)
						{
							return pChildObj->UpdateItemValue(rt, this,
								IdList, id_offset, itemName, val);
						}
					}
				}
				//all elses, return value passed in
				return val;
			}
			SCOPE_FAST_CALL_AddOrGet0(index,m_pMyScope,itemName, true);
			if (index > 0)
			{
				X::Value valMember;
				GetIndexValue(index, valMember);
				//only change Prop's value
				if (valMember.IsObject() &&
					valMember.GetObj()->GetType() == X::ObjType::Prop)
				{
					auto* pPropObj = dynamic_cast<Data::PropObject*>(valMember.GetObj());
					if (pPropObj)
					{
						if (!pPropObj->SetPropValue(rt, this, val))
						{
							//Failed, return the original value
							pPropObj->GetPropValue(rt, this, retVal);
						}
					}
				}
			}
			return retVal;
		}
		void Package::UnloadAddedModules()
		{
			for (auto* pModule : m_loadedModules)
			{
				X::Hosting::I().Unload(pModule);
			}
			m_loadedModules.clear();
		}
		int Package::GetPackageName(char* buffer, int bufferSize)
		{
			APISetBase* pAPISet = (APISetBase*)GetAPISet();
			if(pAPISet)
			{
				std::string  strName(pAPISet->GetName());
				if (strName.size() <= bufferSize)
				{
					strcpy(buffer, strName.c_str());
					return strName.size();
				}
			}
			return 0;
		}
		bool Package::RunCodeWithThisScope(const char* code)
		{
			auto* pTopModule = X::Hosting::I().LoadWithScope(m_pMyScope, code,strlen(code));
			m_loadedModules.push_back(pTopModule);
			//change StackFrame VarCount
			m_variableFrame->SetVarCount(m_pMyScope->GetVarNum());
			X::Value retVal;
			std::vector<X::Value> passInParams;
			bool bOK = X::Hosting::I().Run(pTopModule, retVal, passInParams);
			return bOK;
		}
		X::Data::List* Package::FlatPack(XlangRuntime* rt, XObj* pContext,
			std::vector<std::string>& IdList, int id_offset,
			long long startIndex, long long count)
		{
			AutoLock autoLock(m_lock);
			if (id_offset < IdList.size())
			{
				auto& key = IdList[id_offset++];
				X::Value item;
				SCOPE_FAST_CALL_AddOrGet0(idx,m_pMyScope,key, true);
				if (idx >= 0)
				{
					GetIndexValue(idx, item);
					if (item.IsObject())
					{
						Object* pChildObj = dynamic_cast<Object*>(item.GetObj());
						if (pChildObj)
						{
							return pChildObj->FlatPack(rt, pContext, IdList, id_offset, startIndex, count);
						}
					}
				}
				//all elses, return an empty list
				X::Data::List* pOutList = new X::Data::List();
				pOutList->IncRef();
				return pOutList;
			}
			X::Data::List* pOutList = new X::Data::List();
			pOutList->IncRef();

			auto& memberInfo = GetMemberInfo();
			for (auto& it : memberInfo)
			{
				X::Data::Dict* dict = new X::Data::Dict();
				Data::Str* pStrName = new Data::Str(it.name);
				dict->Set("Name", X::Value(pStrName));
				//make object_ids
				std::string myId = it.name;
				auto valType = GetMemberType(it.type);
				SCOPE_FAST_CALL_AddOrGet0(idx,m_pMyScope,it.name, true);
				X::Value val;
				GetIndexValue(idx, val);
				if (val.IsObject() && val.GetObj()->GetType()
					== X::ObjType::Prop)
				{
					auto* pPropObj = dynamic_cast<X::Data::PropObject*>(val.GetObj());
					if (pPropObj)
					{
						//if this Prop is an Object,can't hold this object
						//because no chance to release it
						//so keep as PropObject which has Refcount hold by its owner
						//but for others, replace with this value
						X::Value v0;
						pPropObj->GetPropValue(rt, this, v0);
						if (!v0.IsObject() || v0.GetObj()->GetType() == X::ObjType::Str)
						{
							val = v0;
							valType = val.GetValueType();
						}
					}
				}
				auto objIds = CombinObjectIds(IdList, myId);
				dict->Set("Id", objIds);
				Data::Str* pStrType = new Data::Str(valType);
				dict->Set("Type", X::Value(pStrType));
				if (!val.IsObject() || (val.IsObject()
					&& dynamic_cast<Object*>(val.GetObj())->IsStr()))
				{
					dict->Set("Value", val);
				}
				else if (it.type == PackageMemberType::Func ||
					it.type == PackageMemberType::FuncEx)
				{
					dict->Set("Value", it.doc);
				}
				else if (val.IsObject())
				{
					X::Value objId((unsigned long long)val.GetObj());
					dict->Set("Value", objId);
					if (val.GetObj()->GetType() == X::ObjType::Prop)
					{
						auto* pPropObj = dynamic_cast<X::Data::PropObject*>(val.GetObj());
						X::Value v0;
						pPropObj->GetPropValue(rt, this, v0);
						long long objSize = v0.Size();
						X::Value valSize(objSize);
						dict->Set("Size", valSize);
					}
					else
					{
						X::Value valShape = val.GetObj()->Shapes();
						if (valShape.IsList())
						{
							dict->Set("Size", valShape);
						}
						else
						{
							X::Value valSize(val.GetObj()->Size());
							dict->Set("Size", valSize);
						}
					}
				}
				X::Value valDict(dict);
				pOutList->Add(rt, valDict);
			}
			return pOutList;
		}
		X::Value PackageProxy::UpdateItemValue(XlangRuntime* rt, XObj* pContext,
			std::vector<std::string>& IdList, int id_offset,
			std::string itemName, X::Value& val)
		{
			X::Value retVal = val;
			AutoLock autoLock(Object::m_lock);
			if (id_offset < IdList.size())
			{
				auto& strId = IdList[id_offset++];
				X::Value item;
				SCOPE_FAST_CALL_AddOrGet0(idx,m_pPackage->GetMyScope(),strId, true);
				if (idx >= 0)
				{
					GetIndexValue(idx, item);
					if (item.IsObject())
					{
						Object* pChildObj = dynamic_cast<Object*>(item.GetObj());
						if (pChildObj)
						{
							return pChildObj->UpdateItemValue(rt, this,
								IdList, id_offset, itemName, val);
						}
					}
				}
				//all elses, return value passed in
				return val;
			}
			SCOPE_FAST_CALL_AddOrGet0(index,m_pPackage->GetMyScope(),itemName, true);
			if (index > 0)
			{
				X::Value valMember;
				GetIndexValue(index, valMember);
				//only change Prop's value
				if (valMember.IsObject() &&
					valMember.GetObj()->GetType() == X::ObjType::Prop)
				{
					auto* pPropObj = dynamic_cast<Data::PropObject*>(valMember.GetObj());
					if (pPropObj)
					{
						if (!pPropObj->SetPropValue(rt, this, val))
						{
							//Failed, return the original value
							pPropObj->GetPropValue(rt, this, retVal);
						}
					}
				}
			}
			return retVal;
		}
		bool Package::ToBytesImpl(XlangRuntime* rt, void* pEmbededObject, X::XLangStream& stream)
		{
			//store the pEmbededObject as Package's ID in ScopeSpace
			//use this way to restore Package Object with same pEmbededObject
			unsigned long long embededId = (unsigned long long)pEmbededObject;
			stream << embededId;
			auto* pRefPackObj = stream.ScopeSpace().Query(embededId);
			bool bRefPackObj = (pRefPackObj != nullptr);
			stream << bRefPackObj;
			if (pRefPackObj)
			{
				return true;
			}
			//if first one, put into scopespacem and continue to store its content
			stream.ScopeSpace().Add(embededId,(void*)this);

			APISetBase* pAPISet = (APISetBase*)GetAPISet();
			std::string strPackUri;

			//encoding uri for this package
			APISetBase* pAPISet0 = pAPISet;
			void* pCurEmbededObj = pEmbededObject;
			//Uri ={packName.[instanceIdentity]}+
			while (pAPISet0)
			{
				std::string uriItem = pAPISet0->GetName();
				if (pAPISet0->GetInstanceProc())
				{
					auto varInstanceIdentity = pAPISet0->GetInstanceProc()(pCurEmbededObj);
					std::string strInstanceIdentity = varInstanceIdentity.ToString();
					uriItem += "." + strInstanceIdentity;
				}

				if (strPackUri.empty())
				{
					strPackUri = uriItem;
				}
				else
				{
					strPackUri = uriItem + '|' + strPackUri;
				}
				APISetBase* pAPISet1 = pAPISet0->GetParent();
				if (pAPISet0->GetEmbededParentObjectProc())
				{
					pCurEmbededObj = pAPISet0->GetEmbededParentObjectProc()(pCurEmbededObj);
				}
				//for top package, append its libname
				if (pAPISet1 == nullptr)
				{
					strPackUri = std::string(pAPISet0->GetLibName()) + '|' + strPackUri;
					break;
				}
				else
				{
					pAPISet0 = pAPISet1;
				}
			}
			//pack URI
			stream << strPackUri;
			//pack size if no this package loaded, can skip this package's content
			long long size = 0;
			if (pAPISet)
			{
				auto sizeFunc = pAPISet->GetSizeFunc();
				if (sizeFunc)
				{
					size = sizeFunc(pEmbededObject);
				}
			}
			stream << size;
			if (pAPISet)
			{
				auto toBytesFunc = pAPISet->GetToBytesFunc();
				if (toBytesFunc)
				{
					toBytesFunc(pEmbededObject, &stream);
				}
			}


			return true;
		}
		bool Package::FromBytesImpl(void* pEmbededObject,X::XLangStream& stream)
		{
			bool bOK = true;
			long long size = 0;
			stream >> size;
			APISetBase* pAPISet = (APISetBase*)GetAPISet();
			if (pAPISet)
			{
				auto fromBytesFunc = pAPISet->GetFromBytesFunc();
				if (fromBytesFunc)
				{
					bOK = fromBytesFunc(pEmbededObject, &stream);
				}
			}
			return bOK;
		}
		bool PackageProxy::Call(XRuntime* rt, XObj* pContext, 
			ARGS& params, KWARGS& kwParams, X::Value& retValue)
		{
			auto func = m_pPackage->GetCall();
			if(func)
			{
				return func(rt,this, pContext, params, kwParams, retValue);
			}
			return true;
		}
		X::Data::List* PackageProxy::FlatPack(XlangRuntime* rt, XObj* pContext,
			std::vector<std::string>& IdList, int id_offset,
			long long startIndex, long long count)
		{
			AutoLock autoLock(Object::m_lock);
			if (id_offset < IdList.size())
			{
				auto& strId = IdList[id_offset++];
				X::Value item;
				SCOPE_FAST_CALL_AddOrGet0(idx,m_pPackage->GetMyScope(),strId, true);
				if (idx >= 0)
				{
					GetIndexValue(idx, item);
					if (item.IsObject())
					{
						Object* pChildObj = dynamic_cast<Object*>(item.GetObj());
						if (pChildObj)
						{
							return pChildObj->FlatPack(rt, this, IdList, id_offset, startIndex, count);
						}
					}
				}
				//all elses, return an empty list
				X::Data::List* pOutList = new X::Data::List();
				pOutList->IncRef();
				return pOutList;
			}
			X::Data::List* pOutList = new X::Data::List();
			pOutList->IncRef();
			if (m_pPackage == nullptr)
			{
				return pOutList;
			}
			auto& memberInfo = m_pPackage->GetMemberInfo();
			for (auto& it : memberInfo)
			{
				X::Data::Dict* dict = new X::Data::Dict();
				Data::Str* pStrName = new Data::Str(it.name);
				dict->Set("Name", X::Value(pStrName));
				//make object_ids
				std::string myId = it.name;
				SCOPE_FAST_CALL_AddOrGet0(idx,m_pPackage->GetMyScope(),it.name, true);
				auto valType = GetMemberType(it.type);
				X::Value val;
				GetIndexValue(idx, val);
				if (val.IsObject())
				{
					if (val.GetObj()->GetType() == X::ObjType::Prop)
					{
						auto* pPropObj = dynamic_cast<X::Data::PropObject*>(val.GetObj());
						if (pPropObj)
						{
							//if this Prop is an Object,can't hold this object
							//because no chance to release it
							//so keep as PropObject which has Refcount hold by its owner
							//but for others, replace with this value
							X::Value v0;
							pPropObj->GetPropValue(rt, this, v0);
							if (!v0.IsObject() || v0.GetObj()->GetType() == X::ObjType::Str)
							{
								val = v0;
								valType = val.GetValueType();
							}
						}
					}
					else if (val.GetObj()->GetType() == X::ObjType::ObjectEvent)
					{
						auto* pEvtObj = dynamic_cast<X::ObjectEvent*>(val.GetObj());
						if (pEvtObj)
						{
							auto str_abi = pEvtObj->ToString();
							std::string strVal = str_abi;
							X::g_pXHost->ReleaseString(str_abi);
							val = strVal;
						}
					}
				}
				auto objIds = CombinObjectIds(IdList, myId);
				dict->Set("Id", objIds);

				Data::Str* pStrType = new Data::Str(valType);
				dict->Set("Type", X::Value(pStrType));
				if (!val.IsObject() || (val.IsObject()
					&& dynamic_cast<Object*>(val.GetObj())->IsStr()))
				{
					dict->Set("Value", val);
				}
				else if (it.type == PackageMemberType::Func ||
					it.type == PackageMemberType::FuncEx)
				{
					dict->Set("Value", it.doc);
				}
				else if (val.IsObject())
				{
					X::Value objId((unsigned long long)val.GetObj());
					dict->Set("Value", objId);
					if (val.GetObj()->GetType() == X::ObjType::Prop)
					{
						auto* pPropObj = dynamic_cast<X::Data::PropObject*>(val.GetObj());
						X::Value v0;
						pPropObj->GetPropValue(rt, this, v0);
						long long objSize = v0.Size();
						X::Value valSize(objSize);
						dict->Set("Size", valSize);
					}
					else
					{
						X::Value valShape = val.GetObj()->Shapes();
						if (valShape.IsList())
						{
							dict->Set("Size", valShape);
						}
						else
						{
							X::Value valSize(val.GetObj()->Size());
							dict->Set("Size", valSize);
						}
					}
				}
				X::Value valDict(dict);
				pOutList->Add(rt, valDict);
			}

			return pOutList;
		}
	}
}