#include "RemoteObjectStub.h"
#include "StubMgr.h"
#include "bin.h"
#include "package.h"

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
			(unsigned int)RPC_CALL_TYPE::CantorProxy_GetMemberObject,
			this,
			std::string("GetMemberObject"),
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
			m_rt = X::g_pXHost->CreateRuntime();
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
		X::XObj* pObjRet = (X::XObj*)id;
		return pObjRet;
	}

	X::ROBJ_ID RemoteObjectStub::ConvertXObjToId(X::XObj* obj)
	{
		obj->IncRef();
		return X::ROBJ_ID(obj);
	}
	bool RemoteObjectStub::QueryRootObject(int channel,
		SwapBufferStream& stream, RemotingProc* pProc)
	{
		std::string objName;
		stream >> objName;
		pProc->NotifyBeforeCall(channel, stream);
		auto pXObj = QueryObjWithName(objName);
		bool bOK = (pXObj!=nullptr);
		pProc->NotifyAfterCall(channel, stream, bOK);
		if (bOK)
		{
			X::ROBJ_ID objId = ConvertXObjToId(pXObj);
			stream << objId;
		}
		return true;
	}
	bool RemoteObjectStub::QueryMember(int channel,
		SwapBufferStream& stream, RemotingProc* pProc)
	{
		X::ROBJ_ID objId;
		std::string name;
		stream >> objId;
		stream >> name;
		pProc->NotifyBeforeCall(channel, stream);
		auto pXObj = CovertIdToXObj(objId);
		auto pXPack = dynamic_cast<X::AST::Package*>(pXObj);
		auto memberInfo = pXPack->QueryMethod(name);
		pProc->NotifyAfterCall(channel, stream, true);
		stream << memberInfo.Index;
		stream << memberInfo.KeepRawParams;
		return true;
	}

	bool RemoteObjectStub::GetMemberObject(int channel, SwapBufferStream& stream, RemotingProc* pProc)
	{
		X::ROBJ_ID objId;
		X::ROBJ_MEMBER_ID memId;
		stream >> objId;
		stream >> memId;
		pProc->NotifyBeforeCall(channel, stream);
		auto pXObj = CovertIdToXObj(objId);
		auto pXPack = dynamic_cast<X::XPackage*>(pXObj);
		X::Value valObj;
		bool bOK = pXPack->GetIndexValue(memId, valObj);
		pProc->NotifyAfterCall(channel, stream, bOK);
		if (bOK)
		{
			X::ROBJ_ID sub_objId = ConvertXObjToId(valObj.GetObj());
			stream << sub_objId;
		}
		return true;
	}

	bool RemoteObjectStub::RCall(int channel, SwapBufferStream& stream, RemotingProc* pProc)
	{
		X::ROBJ_ID parent_ObjId;
		X::ROBJ_ID objId;
		X::ROBJ_MEMBER_ID memId;
		int argNum = 0;
		stream >> parent_ObjId;
		stream >> objId;
		stream >> memId;
		stream >> argNum;
		X::ARGS params;
		for (int i = 0; i < argNum; i++)
		{
			X::Value v;
			v.FromBytes(&stream);
			params.push_back(v);
		}
		int kwNum = 0;
		stream >> kwNum;
		X::KWARGS kwParams;
		for (int i = 0; i < kwNum; i++)
		{
			std::string key;
			stream >> key;
			X::Value v;
			v.FromBytes(&stream);
			kwParams.emplace(std::make_pair(key, v));
		}
		bool haveTrailer = false;
		stream >> haveTrailer;
		X::Value trailer;
		if (haveTrailer)
		{
			auto size0 = stream.CalcSize(stream.GetPos());
			auto size1 = stream.CalcSize();
			auto trailerSize = size1 - size0;
			char* pBinBuf = new char[trailerSize];
			stream.CopyTo(pBinBuf, trailerSize);
			Data::Binary* pTrailerBin = new Data::Binary(pBinBuf, trailerSize);
			trailer = pTrailerBin;
		}
		pProc->NotifyBeforeCall(channel, stream);
		X::XObj* pParentObj = nullptr;
		if (parent_ObjId != nullptr)
		{
			pParentObj = CovertIdToXObj(parent_ObjId);
		}
		auto pXObj = CovertIdToXObj(objId);
		X::Value valRet;

		bool bOK = false;
		if (haveTrailer)
		{
			bOK = pXObj->CallEx(m_rt, pParentObj, params, kwParams, trailer,valRet);
		}
		else
		{
			bOK = pXObj->Call(m_rt, pParentObj, params, kwParams, valRet);
		}
		pProc->NotifyAfterCall(channel, stream, bOK);
		if (bOK)
		{
			X::ROBJ_ID retId = 0;
			if (valRet.IsObject())
			{
				auto pRetObj = valRet.GetObj();
				if (pRetObj->GetType() == X::ObjType::Package)
				{
					retId = ConvertXObjToId(pRetObj);
				}
			}
			stream << retId;
			if (retId == 0)
			{//if not XPackage, return as value
				valRet.ToBytes(&stream);
			}
		}
		return true;
	}

	bool RemoteObjectStub::Call(int channel, unsigned int callId,
		SwapBufferStream& stream, RemotingProc* pProc)
	{
		bool bOK = false;
		RPC_CALL_TYPE callType = (RPC_CALL_TYPE)callId;
		switch (callType)
		{
		case RPC_CALL_TYPE::CantorProxy_QueryRootObject:
			bOK = QueryRootObject(channel, stream, pProc);
			break;
		case RPC_CALL_TYPE::CantorProxy_QueryMember:
			bOK = QueryMember(channel, stream, pProc);
			break;
		case RPC_CALL_TYPE::CantorProxy_GetMemberObject:
			bOK = GetMemberObject(channel, stream, pProc);
			break;
		case RPC_CALL_TYPE::CantorProxy_Call:
			bOK = RCall(channel, stream, pProc);
			break;
		default:
			break;
		}
		return bOK;

	}
}