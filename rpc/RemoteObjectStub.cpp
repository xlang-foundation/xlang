#include "RemoteObjectStub.h"
#include "StubMgr.h"
#include "bin.h"
#include "package.h"
#include "utility.h"
#include "remote_object.h"
#include "list.h"
#include "prop.h"
#include "remote_client_object.h"

namespace X
{
	RemoteObjectStub::RemoteObjectStub()
	{

	}

	RemoteObjectStub::~RemoteObjectStub()
	{
	}

	void RemoteObjectStub::Register()
	{
		RemotingManager::I().Register(
			(unsigned int)RPC_CALL_TYPE::CantorProxy_QueryRootObject,
			this,
			std::string("QueryRootObject"),
			std::vector<std::string>{},
			std::string("bool")
		);
		RemotingManager::I().Register(
			(unsigned int)RPC_CALL_TYPE::CantorProxy_QueryMember,
			this,
			std::string("QueryMember"),
			std::vector<std::string>{},
			std::string("bool")
		);
		RemotingManager::I().Register(
			(unsigned int)RPC_CALL_TYPE::CantorProxy_QueryMemberCount,
			this,
			std::string("QueryMemberCount"),
			std::vector<std::string>{},
			std::string("int")
		);
		RemotingManager::I().Register(
			(unsigned int)RPC_CALL_TYPE::CantorProxy_FlatPack,
			this,
			std::string("FlatPack"),
			std::vector<std::string>{},
			std::string("int")
		);
		RemotingManager::I().Register(
			(unsigned int)RPC_CALL_TYPE::CantorProxy_UpdateItemValue,
			this,
			std::string("UpdateItemValue"),
			std::vector<std::string>{},
			std::string("int")
		);
		RemotingManager::I().Register(
			(unsigned int)RPC_CALL_TYPE::CantorProxy_GetMemberObject,
			this,
			std::string("GetMemberObject"),
			std::vector<std::string>{},
			std::string("bool")
		);
		RemotingManager::I().Register(
			(unsigned int)RPC_CALL_TYPE::CantorProxy_ReleaseObject,
			this,
			std::string("ReleaseObject"),
			std::vector<std::string>{},
			std::string("bool")
		);
		RemotingManager::I().Register(
			(unsigned int)RPC_CALL_TYPE::CantorProxy_Call,
			this,
			std::string("Call"),
			std::vector<std::string>{},
			std::string("bool")
		);
	}

	void RemoteObjectStub::EnsureRT()
	{
		if (m_rt == nullptr)
		{
			m_rt = X::g_pXHost->CreateRuntime(true);
		}
	}
	X::XObj* RemoteObjectStub::QueryObjWithName(std::string& name)
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
	X::XObj* RemoteObjectStub::CovertIdToXObj(X::ROBJ_ID id)
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

	X::ROBJ_ID RemoteObjectStub::ConvertXObjToId(X::XObj* obj)
	{
		obj->IncRef();
		auto pid = GetPID();
		return X::ROBJ_ID{ pid,obj};
	}
	bool RemoteObjectStub::QueryRootObject(void* pCallContext, SwapBufferStream& stream, RemotingProc* pProc)
	{
		std::string objName;
		stream >> objName;
		pProc->NotifyBeforeCall(stream);
		auto pXObj = QueryObjWithName(objName);
		bool bOK = (pXObj!=nullptr);
		pProc->NotifyAfterCall(stream, bOK);
		if (bOK)
		{
			X::ROBJ_ID objId = ConvertXObjToId(pXObj);
			stream << objId;
		}
		pProc->FinishCall(pCallContext,stream, bOK);
		return true;
	}
	bool RemoteObjectStub::QueryMember(void* pCallContext, SwapBufferStream& stream, RemotingProc* pProc)
	{
		X::ROBJ_ID objId;
		std::string name;
		stream >> objId;
		stream >> name;
		pProc->NotifyBeforeCall(stream);
		auto pXObj = CovertIdToXObj(objId);
		int flags = 0;
		int idx = pXObj->QueryMethod(name.c_str(),&flags);
		pProc->NotifyAfterCall(stream, true);
		stream << idx;
		stream << flags;
		pProc->FinishCall(pCallContext, stream, true);
		return true;
	}
	bool RemoteObjectStub::QueryMemberCount(void* pCallContext, SwapBufferStream& stream, RemotingProc* pProc)
	{
		X::ROBJ_ID objId;
		stream >> objId;
		pProc->NotifyBeforeCall(stream);
		auto pXObj = CovertIdToXObj(objId);
		long long size = 0;
		bool bOK = false;
		if (pXObj)
		{
			size = pXObj->Size();
			bOK = true;
		}
		pProc->NotifyAfterCall(stream, bOK);
		if (bOK)
		{
			stream << size;
		}
		pProc->FinishCall(pCallContext, stream, bOK);
		return true;
	}
	bool RemoteObjectStub::FlatPack(void* pCallContext, SwapBufferStream& stream, RemotingProc* pProc)
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
		pProc->NotifyBeforeCall(stream);
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
					pParentObj, IdList, id_offset,startIndex, count);
				valPackList = X::Value(dynamic_cast<XObj*>(pPackList), false);
			}
			else
			{
				auto* pPackageProxy = dynamic_cast<X::AST::PackageProxy*>(pXObj);
				if (pPackageProxy)
				{
					auto* pPackList = pPackageProxy->FlatPack((XlangRuntime*)m_rt, 
						pParentObj, IdList, id_offset,startIndex, count);
					valPackList = X::Value(dynamic_cast<XObj*>(pPackList), false);
				}
			}
		}
		pProc->NotifyAfterCall(stream, bOK);
		if (bOK)
		{
			stream << valPackList;
		}
		pProc->FinishCall(pCallContext, stream, bOK);
		return true;
	}
	bool RemoteObjectStub::UpdateItemValue(void* pCallContext, SwapBufferStream& stream, RemotingProc* pProc)
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
		pProc->NotifyBeforeCall(stream);
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
					pParentObj, IdList, id_offset, itemName, newVal);
			}
			else
			{
				auto* pPackageProxy = dynamic_cast<X::AST::PackageProxy*>(pXObj);
				if (pPackageProxy)
				{
					retVal = pPackageProxy->UpdateItemValue((XlangRuntime*)m_rt,
						pParentObj, IdList, id_offset, itemName, newVal);
				}
			}
		}
		pProc->NotifyAfterCall(stream, bOK);
		if (bOK)
		{
			stream << retVal;
		}
		pProc->FinishCall(pCallContext, stream, bOK);
		return true;
	}
	bool RemoteObjectStub::GetMemberObject(void* pCallContext, SwapBufferStream& stream, RemotingProc* pProc)
	{
		X::ROBJ_ID objId;
		X::ROBJ_MEMBER_ID memId;
		stream >> objId;
		stream >> memId;
		pProc->NotifyBeforeCall(stream);
		auto pXObj = CovertIdToXObj(objId);
		X::Value valObj;
		bool bOK = pXObj->GetIndexValue(memId, valObj);
		pProc->NotifyAfterCall(stream, bOK);
		if (bOK)
		{
			X::ROBJ_ID sub_objId = ConvertXObjToId(valObj.GetObj());
			stream << sub_objId;
		}
		pProc->FinishCall(pCallContext, stream, bOK);
		return true;
	}
	bool RemoteObjectStub::ReleaseObject(void* pCallContext, SwapBufferStream& stream, RemotingProc* pProc)
	{
		X::ROBJ_ID objId;
		stream >> objId;
		pProc->NotifyBeforeCall(stream);
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
		pProc->NotifyAfterCall(stream, true);
		pProc->FinishCall(pCallContext, stream, true);
		return true;
	}

	struct CallInfo
	{
		X::ROBJ_ID parent_ObjId;
		X::ROBJ_ID objId;
		X::ARGS params;
		X::KWARGS kwParams;
		bool haveTrailer;
		X::Value trailer;
		CallInfo():params(0)
		{

		}
	};
	bool RemoteObjectStub::RCall(void* pCallContext, SwapBufferStream& stream, RemotingProc* pProc)
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
			if (v.IsObject() && v.GetObj()->GetType() == X::ObjType::RemoteClientObject)
			{
				auto* pRemoteClientObj = dynamic_cast<X::RemoteClientObject*>(v.GetObj());
				pRemoteClientObj->SetStub((XLangStub*)pProc);
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
			if (v.IsObject() && v.GetObj()->GetType() == X::ObjType::RemoteClientObject)
			{
				auto* pRemoteClientObj = dynamic_cast<X::RemoteClientObject*>(v.GetObj());
				pRemoteClientObj->SetStub((XLangStub*)pProc);
			}
			pCallInfo->kwParams.Add(key.c_str(), v,true);
		}
		stream >> pCallInfo->haveTrailer;
		if (pCallInfo->haveTrailer)
		{
			auto size0 = stream.CalcSize(stream.GetPos());
			auto size1 = stream.CalcSize();
			auto trailerSize = size1 - size0;
			char* pBinBuf = new char[trailerSize];
			stream.CopyTo(pBinBuf, trailerSize);
			Data::Binary* pTrailerBin = new Data::Binary(pBinBuf, trailerSize,true);
			pCallInfo->trailer = pTrailerBin;
		}
		pProc->NotifyBeforeCall(stream);

		auto callProc = [this, pCallInfo, pProc, pCallContext]()
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
			SwapBufferStream stream;
			pProc->NotifyAfterCall(stream, bOK);
			if (bOK)
			{
				X::ROBJ_ID retId = { 0,0 };
				if (valRet.IsObject())
				{
					auto tp = valRet.GetObj()->GetType();
					//for str and Bin object, directly put into stream
					if (tp != ObjType::Str && tp != ObjType::Binary && tp != ObjType::List)
					{
						auto pRetObj = valRet.GetObj();
						retId = ConvertXObjToId(pRetObj);
					}
					else if (tp == ObjType::List && valRet.Size() > LIST_PASS_PROCESS_SIZE)
					{
						auto pRetObj = valRet.GetObj();
						retId = ConvertXObjToId(pRetObj);
					}
				}
				stream << retId;
				if (retId.objId == 0)
				{//if not XPackage, return as value
					valRet.ToBytes(&stream);
				}
			}
			pProc->FinishCall(pCallContext, stream, bOK);
			pProc->Release();
		};
		pProc->AddRef();//add a ref for call in thread,
		//will call release end of callProc
		auto* pWorker = RemotingManager::I().GetIdleCallWorker();
		pWorker->Call(callProc);
		return true;
	}

	bool RemoteObjectStub::Call(void* pCallContext, unsigned int nCallType,
		SwapBufferStream& stream, RemotingProc* pProc)
	{
		bool bOK = false;
		pProc->AddRef();
		RPC_CALL_TYPE callType = (RPC_CALL_TYPE)nCallType;
		switch (callType)
		{
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
			bOK = GetMemberObject(pCallContext,stream, pProc);
			break;
		case RPC_CALL_TYPE::CantorProxy_ReleaseObject:
			bOK = ReleaseObject(pCallContext,stream, pProc);
			break;
		case RPC_CALL_TYPE::CantorProxy_Call:
			bOK = RCall(pCallContext,stream, pProc);
			break;
		default:
			break;
		}
		pProc->Release();
		return bOK;
	}
	bool RemoteObjectStub::ExtractNativeObjectFromRemoteObject(
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