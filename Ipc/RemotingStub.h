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

#pragma once

#include "singleton.h"
#include "service_def.h"
#include "xproxy.h"
#include "xhost.h"
#include "xlang.h"
#include "utility.h"
#include "IpcPool.h"
#include "list.h"
#include "bin.h"
#include "remote_object.h"
#include "RemotingMethod.h"
#include "prop.h"

namespace X
{
	namespace IPC
	{
		struct CallInfo
		{
			X::ROBJ_ID parent_ObjId;
			X::ROBJ_ID objId;
			X::ARGS params;
			X::KWARGS kwParams;
			bool haveTrailer;
			X::Value trailer;
			CallInfo() :params(0)
			{

			}
		};
		class RemotingStub :
			public RemotingCallBase,
			public Singleton<RemotingStub>
		{
		public:
			RemotingStub() :threadPool(15)
			{
			}
			void Register();
			bool ExtractNativeObjectFromRemoteObject(
				X::Value& remoteObj,
				X::Value& nativeObj);

		private:
			template<class F, class... Args>
			auto AddTask(F&& f, Args&&... args) -> std::future<typename std::invoke_result<F, Args...>::type>;
			IPC::ThreadPool threadPool;

			X::XRuntime* m_rt = nullptr;
			void EnsureRT()
			{
				if (m_rt == nullptr)
				{
					m_rt = X::g_pXHost->CreateRuntime(true);
				}
			}
			X::XObj* QueryObjWithName(std::string& name);
			X::XObj* CovertIdToXObj(X::ROBJ_ID);
			X::ROBJ_ID ConvertXObjToId(X::XObj* obj)
			{
				//TODO: for remote object, do we need to inc ref?
				if (obj->GetType() == X::ObjType::RemoteObject)
				{
					auto* pRemoteObj = dynamic_cast<X::RemoteObject*>(obj);
					return pRemoteObj->GetObjId();
				}
				else
				{
					obj->IncRef();
					auto pid = GetPID();
					return X::ROBJ_ID{ pid,obj };
				}
			}
			bool QueryRootObject(void* pCallContext, SwapBufferStream& stream, RemotingProc* pProc);
			bool QueryMember(void* pCallContext, SwapBufferStream& stream, RemotingProc* pProc);
			bool QueryMemberCount(void* pCallContext, SwapBufferStream& stream, RemotingProc* pProc);
			bool FlatPack(void* pCallContext, SwapBufferStream& stream, RemotingProc* pProc);
			bool UpdateItemValue(void* pCallContext, SwapBufferStream& stream, RemotingProc* pProc);
			bool GetMemberObject(void* pCallContext, SwapBufferStream& stream, RemotingProc* pProc);
			bool ReleaseObject(void* pCallContext, SwapBufferStream& stream, RemotingProc* pProc);
			bool RCall(void* pCallContext, SwapBufferStream& stream, RemotingProc* pProc);
			// Inherited via RemotingCallBase
			virtual bool Call(void* pCallContext, unsigned int callType, SwapBufferStream& stream, RemotingProc* pProc) override;
		};
		//Implementations
		template<class F, class... Args>
		auto RemotingStub::AddTask(F&& f, Args&&... args) -> std::future<typename std::invoke_result<F, Args...>::type>
		{
			return threadPool.enqueue(std::forward<F>(f), std::forward<Args>(args)...);
		}
		void RemotingStub::Register()
		{
			RemotingMethod::I().Register(
				(unsigned int)RPC_CALL_TYPE::ShakeHands,
				this,
				std::string("ShakeHands"),
				std::vector<std::string>{},
				std::string("void")
			);
			RemotingMethod::I().Register(
				(unsigned int)RPC_CALL_TYPE::CantorProxy_QueryRootObject,
				this,
				std::string("QueryRootObject"),
				std::vector<std::string>{},
				std::string("bool")
			);
			RemotingMethod::I().Register(
				(unsigned int)RPC_CALL_TYPE::CantorProxy_QueryMember,
				this,
				std::string("QueryMember"),
				std::vector<std::string>{},
				std::string("bool")
			);
			RemotingMethod::I().Register(
				(unsigned int)RPC_CALL_TYPE::CantorProxy_QueryMemberCount,
				this,
				std::string("QueryMemberCount"),
				std::vector<std::string>{},
				std::string("int")
			);
			RemotingMethod::I().Register(
				(unsigned int)RPC_CALL_TYPE::CantorProxy_FlatPack,
				this,
				std::string("FlatPack"),
				std::vector<std::string>{},
				std::string("int")
			);
			RemotingMethod::I().Register(
				(unsigned int)RPC_CALL_TYPE::CantorProxy_UpdateItemValue,
				this,
				std::string("UpdateItemValue"),
				std::vector<std::string>{},
				std::string("int")
			);
			RemotingMethod::I().Register(
				(unsigned int)RPC_CALL_TYPE::CantorProxy_GetMemberObject,
				this,
				std::string("GetMemberObject"),
				std::vector<std::string>{},
				std::string("bool")
			);
			RemotingMethod::I().Register(
				(unsigned int)RPC_CALL_TYPE::CantorProxy_ReleaseObject,
				this,
				std::string("ReleaseObject"),
				std::vector<std::string>{},
				std::string("bool")
			);
			RemotingMethod::I().Register(
				(unsigned int)RPC_CALL_TYPE::CantorProxy_Call,
				this,
				std::string("Call"),
				std::vector<std::string>{},
				std::string("bool")
			);
		}
		X::XObj* RemotingStub::QueryObjWithName(std::string& name)
		{
			X::XObj* pObjRet = nullptr;
			EnsureRT();
			X::Value valPack;
			if (X::g_pXHost->QueryPackage(m_rt, name.c_str(), valPack))
			{
				pObjRet = valPack.GetObj();
			}
			return pObjRet;
		}
		X::XObj* RemotingStub::CovertIdToXObj(X::ROBJ_ID id)
		{
			X::XObj* pObjRet = nullptr;
			auto pid = GetPID();
			if (pid != id.pid)
			{
				//todo:
				assert(false);
				//keep as RemoteObject
			}
			else
			{
				pObjRet = (X::XObj*)id.objId;
			}
			return pObjRet;
		}
		bool RemotingStub::QueryRootObject(void* pCallContext, SwapBufferStream& stream, RemotingProc* pProc)
		{
			std::string objName;
			stream >> objName;
			pProc->EndReceiveCall(stream);
			pProc->AddRef();
			Call_Context callContext = *(Call_Context*)pCallContext;
			AddTask([this, objName, callContext, pProc]()
				{
					auto pXObj = QueryObjWithName((std::string&)objName);
					long long returnCode = (pXObj != nullptr) ? 1 : 0;
					auto& wstream = pProc->BeginWriteReturn((void*)&callContext, returnCode);
					if (returnCode > 0)
					{
						X::ROBJ_ID objId = ConvertXObjToId(pXObj);
						wstream << objId;
					}
					pProc->EndWriteReturn((void*)&callContext, returnCode);
					pProc->Release();
				});
			return true;
		}
		bool RemotingStub::QueryMember(void* pCallContext, SwapBufferStream& stream, RemotingProc* pProc)
		{
			X::ROBJ_ID objId;
			std::string name;
			stream >> objId;
			stream >> name;
			pProc->EndReceiveCall(stream);
			pProc->AddRef();
			Call_Context callContext = *(Call_Context*)pCallContext;
			AddTask([this, objId, name, callContext, pProc]()
				{
					auto pXObj = CovertIdToXObj(objId);
					int flags = 0;
					int idx = pXObj->QueryMethod(name.c_str(), &flags);
					auto& wStream = pProc->BeginWriteReturn((void*)&callContext, 1);
					wStream << idx;
					wStream << flags;
					pProc->EndWriteReturn((void*)&callContext, true);
					pProc->Release();
				});
			return true;
		}
		bool RemotingStub::QueryMemberCount(void* pCallContext, SwapBufferStream& stream, RemotingProc* pProc)
		{
			X::ROBJ_ID objId;
			stream >> objId;
			pProc->EndReceiveCall(stream);
			pProc->AddRef();
			Call_Context callContext = *(Call_Context*)pCallContext;
			AddTask([this, objId, callContext, pProc]()
				{
					auto pXObj = CovertIdToXObj(objId);
					long long size = 0;
					bool bOK = false;
					if (pXObj)
					{
						size = pXObj->Size();
						bOK = true;
					}
					auto& wStream = pProc->BeginWriteReturn((void*)&callContext, bOK ? 1 : 0);
					if (bOK)
					{
						wStream << size;
					}
					pProc->EndWriteReturn((void*)&callContext, bOK ? 1 : 0);
					pProc->Release();
				});
			return true;
		}
		bool RemotingStub::FlatPack(void* pCallContext, SwapBufferStream& stream, RemotingProc* pProc)
		{
			X::ROBJ_ID parent_ObjId;
			X::ROBJ_ID objId;
			std::vector<std::string> IdList;
			int id_offset = 0;
			long long startIndex;
			long long count;
			stream >> parent_ObjId;
			stream >> objId;
			int idNum = 0;
			stream >> idNum;
			for (int i = 0; i < idNum; i++)
			{
				std::string s;
				stream >> s;
				IdList.push_back(s);
			}
			stream >> id_offset;
			stream >> startIndex;
			stream >> count;
			pProc->EndReceiveCall(stream);
			pProc->AddRef();
			Call_Context callContext = *(Call_Context*)pCallContext;
			AddTask([this, parent_ObjId, objId, IdList, id_offset, startIndex, count, callContext, pProc]()
				{
					//todo: for IdList, will be destroyed after this function, need to copy it
					X::XObj* pParentObj = nullptr;
					if (parent_ObjId.objId != nullptr)
					{
						pParentObj = CovertIdToXObj(parent_ObjId);
					}
					auto pXObj = CovertIdToXObj(objId);
					X::Value valPackList;
					bool bOK = (pXObj != nullptr);
					if (bOK)
					{
						auto pPackage = dynamic_cast<X::AST::Package*>(pXObj);
						if (pPackage != nullptr)
						{
							auto* pPackList = pPackage->FlatPack((XlangRuntime*)m_rt,
								pParentObj, (std::vector<std::string>&)IdList, id_offset, startIndex, count);
							if (pPackList)
							{
								valPackList = X::Value(dynamic_cast<XObj*>(pPackList), false);
							}
						}
						else
						{
							auto* pPackageProxy = dynamic_cast<X::AST::PackageProxy*>(pXObj);
							if (pPackageProxy)
							{
								auto* pPackList = pPackageProxy->FlatPack((XlangRuntime*)m_rt,
									pParentObj, (std::vector<std::string>&)IdList, id_offset, startIndex, count);
								if (pPackList)
								{
									valPackList = X::Value(dynamic_cast<XObj*>(pPackList), false);
								}
							}
						}
					}
					auto& wStream = pProc->BeginWriteReturn((void*)&callContext, bOK);
					if (bOK)
					{
						wStream << valPackList;
					}
					pProc->EndWriteReturn((void*)&callContext, bOK);
					pProc->Release();
				});

			return true;
		}
		bool RemotingStub::UpdateItemValue(void* pCallContext, SwapBufferStream& stream, RemotingProc* pProc)
		{
			X::ROBJ_ID parent_ObjId;
			X::ROBJ_ID objId;
			std::vector<std::string> IdList;
			int id_offset = 0;
			stream >> parent_ObjId;
			stream >> objId;
			int idNum = 0;
			stream >> idNum;
			for (int i = 0; i < idNum; i++)
			{
				std::string s;
				stream >> s;
				IdList.push_back(s);
			}
			stream >> id_offset;
			std::string itemName;
			stream >> itemName;
			X::Value newVal;
			stream >> newVal;
			pProc->EndReceiveCall(stream);
			pProc->AddRef();
			Call_Context callContext = *(Call_Context*)pCallContext;
			AddTask([this, parent_ObjId, objId, IdList, id_offset, itemName, newVal, callContext, pProc]()
				{
					//todo: for idlist, will be destroyed after this function, 
					// need to copy it
					X::XObj* pParentObj = nullptr;
					if (parent_ObjId.objId != nullptr)
					{
						pParentObj = CovertIdToXObj(parent_ObjId);
					}
					auto pXObj = CovertIdToXObj(objId);
					X::Value retVal;
					bool bOK = (pXObj != nullptr);
					if (bOK)
					{
						auto pPackage = dynamic_cast<X::AST::Package*>(pXObj);
						if (pPackage != nullptr)
						{
							retVal = pPackage->UpdateItemValue((XlangRuntime*)m_rt,
								pParentObj, (std::vector<std::string>&)IdList, id_offset, itemName, (X::Value&)newVal);
						}
						else
						{
							auto* pPackageProxy = dynamic_cast<X::AST::PackageProxy*>(pXObj);
							if (pPackageProxy)
							{
								retVal = pPackageProxy->UpdateItemValue((XlangRuntime*)m_rt,
									pParentObj, (std::vector<std::string>&)IdList, id_offset, itemName, (X::Value&)newVal);
							}
						}
					}
					auto& wStream = pProc->BeginWriteReturn((void*)&callContext, bOK);
					if (bOK)
					{
						wStream << retVal;
					}
					pProc->EndWriteReturn((void*)&callContext, bOK);
					pProc->Release();
				});

			return true;
		}
		bool RemotingStub::GetMemberObject(void* pCallContext, SwapBufferStream& stream, RemotingProc* pProc)
		{
			X::ROBJ_ID objId;
			X::ROBJ_MEMBER_ID memId;
			bool bGetOnly = false;
			stream >> objId;
			stream >> memId;
			stream >> bGetOnly;
			pProc->EndReceiveCall(stream);
			pProc->AddRef();
			Call_Context callContext = *(Call_Context*)pCallContext;
			AddTask([this, objId, memId, bGetOnly,callContext, pProc]()
				{
					auto pXObj = CovertIdToXObj(objId);
					X::Value valObj;
					bool returnWithValue = false;//not object for prop case
					bool bOK = pXObj->GetIndexValue(memId, valObj);
					//if bGetOnly is true, means this is a right value,not left value( can set)
					//so we directly get the value of prop
					//but if it is a false, means it is left side value( l-value)
					//directly return PropObject
					if (bGetOnly && valObj.IsObject() 
						&& valObj.GetObj()->GetType() == X::ObjType::Prop)
					{
						//need to fetch the value for non-object or string object
						auto* pProp = dynamic_cast<X::Data::PropObject*>(valObj.GetObj());
						if (pProp)
						{
							pProp->GetPropValue((XlangRuntime*)m_rt, pXObj, valObj);
							if (!valObj.IsObject() || 
								(valObj.IsObject() && valObj.GetObj()->GetType() == X::ObjType::Str))
							{
								returnWithValue = true;
							}
						}
					}
					auto& wStream = pProc->BeginWriteReturn((void*)&callContext, bOK);
					if (bOK)
					{
						wStream << returnWithValue;
						if (returnWithValue)
						{
							wStream << valObj;
						}
						else
						{
							X::ROBJ_ID sub_objId = ConvertXObjToId(valObj.GetObj());
							wStream << sub_objId;
						}
					}
					pProc->EndWriteReturn((void*)&callContext, bOK);
					pProc->Release();
				});
			return true;
		}
		bool RemotingStub::ReleaseObject(void* pCallContext, SwapBufferStream& stream, RemotingProc* pProc)
		{
			X::ROBJ_ID objId;
			stream >> objId;
			pProc->EndReceiveCall(stream);
			pProc->AddRef();
			Call_Context callContext = *(Call_Context*)pCallContext;
			AddTask([this, objId, callContext, pProc]()
			{
				auto pXObj = CovertIdToXObj(objId);
				if (pXObj->GetType() == ObjType::Function)
				{
					Data::Object* pObj = dynamic_cast<Data::Object*>(pXObj);
					if (pObj->Ref() <= 2)
					{
						int x = 1;
					}
				}
				pXObj->DecRef();
				auto& wStream = pProc->BeginWriteReturn((void*)&callContext, true);
				pProc->EndWriteReturn((void*)&callContext, true);
				pProc->Release();
			});
			return true;
		}

		bool RemotingStub::RCall(void* pCallContext, SwapBufferStream& stream, RemotingProc* pProc)
		{
			CallInfo* pCallInfo = new CallInfo();
			X::ROBJ_MEMBER_ID memId;
			int argNum = 0;
			stream >> pCallInfo->parent_ObjId;
			stream >> pCallInfo->objId;
			stream >> memId;
			stream >> argNum;
			pCallInfo->params.resize(argNum);
			for (int i = 0; i < argNum; i++)
			{
				X::Value v;
				v.FromBytes(&stream);
				if (v.IsObject() && v.GetObj()->GetType() == X::ObjType::RemoteObject)
				{
					auto* pRemoteClientObj = dynamic_cast<X::RemoteObject*>(v.GetObj());
					auto* pProxy = dynamic_cast<X::XProxy*>(pProc);
					pRemoteClientObj->SetProxy(pProxy);
				}
				pCallInfo->params.push_back(v);
			}
			int kwNum = 0;
			stream >> kwNum;
			pCallInfo->kwParams.resize(kwNum);
			for (int i = 0; i < kwNum; i++)
			{
				std::string key;
				stream >> key;
				X::Value v;
				v.FromBytes(&stream);
				if (v.IsObject() && v.GetObj()->GetType() == X::ObjType::RemoteObject)
				{
					auto* pRemoteClientObj = dynamic_cast<X::RemoteObject*>(v.GetObj());
					auto* pProxy = dynamic_cast<X::XProxy*>(pProc);
					pRemoteClientObj->SetProxy(pProxy);
				}
				pCallInfo->kwParams.Add(key.c_str(), v, true);
			}
			stream >> pCallInfo->haveTrailer;
			if (pCallInfo->haveTrailer)
			{
				auto size0 = stream.CalcSize(stream.GetPos());
				auto size1 = stream.CalcSize();
				auto trailerSize = size1 - size0;
				char* pBinBuf = new char[trailerSize];
				stream.CopyTo(pBinBuf, trailerSize);
				Data::Binary* pTrailerBin = new Data::Binary(pBinBuf, trailerSize, true);
				pCallInfo->trailer = pTrailerBin;
			}
			pProc->EndReceiveCall(stream);
			pProc->AddRef();
			//need to use copy of pCallInfo, because it will be destroyed after this function
			Call_Context callContext = *(Call_Context*)pCallContext;
			AddTask([this, pCallInfo, pProc, callContext]()
				{
					X::XObj* pParentObj = nullptr;
					if (pCallInfo->parent_ObjId.objId != nullptr)
					{
						pParentObj = CovertIdToXObj(pCallInfo->parent_ObjId);
					}
					auto pXObj = CovertIdToXObj(pCallInfo->objId);
					X::Value valRet;

					bool bOK = false;
					if (pCallInfo->haveTrailer)
					{
						bOK = pXObj->CallEx(m_rt, pParentObj,
							pCallInfo->params, pCallInfo->kwParams, pCallInfo->trailer, valRet);
					}
					else
					{
						bOK = pXObj->Call(m_rt, pParentObj, pCallInfo->params, pCallInfo->kwParams, valRet);
					}
					delete pCallInfo;
					auto& wStream = pProc->BeginWriteReturn((void*)&callContext, bOK);
					if (bOK)
					{
						X::ROBJ_ID retId = { GetPID(),0 };
						if (valRet.IsObject())
						{
							auto tp = valRet.GetObj()->GetType();
							if (tp == ObjType::List && valRet.Size() > LIST_PASS_PROCESS_SIZE)
							{
								auto pRetObj = valRet.GetObj();
								retId = ConvertXObjToId(pRetObj);
							}//for str ,dict,etc., directly put into stream
							else if (tp != ObjType::Str
								&& tp != ObjType::Dict
								&& tp != ObjType::Binary 
								&& tp != ObjType::Set
								&& tp != ObjType::Complex
								&& tp != ObjType::Struct
								&& tp != ObjType::Error
								&& tp != ObjType::Range
								&& tp != ObjType::Tensor
								&& tp != ObjType::Table
								&& tp != ObjType::List)
							{
								auto pRetObj = valRet.GetObj();
								retId = ConvertXObjToId(pRetObj);
							}
						}
						wStream << retId;
						if (retId.objId == 0)
						{//if not XPackage, return as value
							valRet.ToBytes(&wStream);
						}
					}
					pProc->EndWriteReturn((void*)&callContext, bOK);
					pProc->Release();

					//std::cout << "RCall finished, reqId:" << callContext.reqId << std::endl;
				});

			return true;
		}
		bool RemotingStub::Call(void* pCallContext, unsigned int nCallType,
			SwapBufferStream& stream, RemotingProc* pProc)
		{
			bool bOK = false;
			pProc->AddRef();
			RPC_CALL_TYPE callType = (RPC_CALL_TYPE)nCallType;
			switch (callType)
			{
			case RPC_CALL_TYPE::ShakeHands:
				pProc->ShakeHandsCall(pCallContext, stream);
				break;
			case RPC_CALL_TYPE::CantorProxy_QueryRootObject:
				bOK = QueryRootObject(pCallContext, stream, pProc);
				break;
			case RPC_CALL_TYPE::CantorProxy_QueryMember:
				bOK = QueryMember(pCallContext, stream, pProc);
				break;
			case RPC_CALL_TYPE::CantorProxy_QueryMemberCount:
				bOK = QueryMemberCount(pCallContext, stream, pProc);
				break;
			case RPC_CALL_TYPE::CantorProxy_FlatPack:
				bOK = FlatPack(pCallContext, stream, pProc);
				break;
			case RPC_CALL_TYPE::CantorProxy_UpdateItemValue:
				bOK = UpdateItemValue(pCallContext, stream, pProc);
				break;
			case RPC_CALL_TYPE::CantorProxy_GetMemberObject:
				bOK = GetMemberObject(pCallContext, stream, pProc);
				break;
			case RPC_CALL_TYPE::CantorProxy_ReleaseObject:
				bOK = ReleaseObject(pCallContext, stream, pProc);
				break;
			case RPC_CALL_TYPE::CantorProxy_Call:
				bOK = RCall(pCallContext, stream, pProc);
				break;
			default:
				break;
			}
			pProc->Release();
			return bOK;
		}
		bool RemotingStub::ExtractNativeObjectFromRemoteObject(
			X::Value& remoteObj,
			X::Value& nativeObj)
		{
			if (!remoteObj.IsObject())
			{
				return false;
			}
			auto* pRemoteObj = dynamic_cast<X::RemoteObject*>(remoteObj.GetObj());
			if (pRemoteObj == nullptr)
			{
				return false;
			}
			auto pid = GetPID();
			auto objId = pRemoteObj->GetObjId();
			if (objId.pid == pid)
			{
				//this object's native object is in this process
				auto pXObj = CovertIdToXObj(objId);
				auto pXPack = dynamic_cast<X::XPackage*>(pXObj);
				nativeObj = pXPack;
			}
			else
			{
				nativeObj = remoteObj;
			}
			return true;
		}
	}
}