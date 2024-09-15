#pragma once
#include "SwapBufferStream.h"
#include "SMSwapBuffer.h"
#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <cassert>
#include "RemotingMethod.h"
#include "IpcBase.h"
#include "wait.h"
#include "gthread.h"
#include "XProxy.h"
#include "remote_object.h"
#include "CallCounter.h"
#include <iostream>

namespace X
{
	namespace IPC
	{

		class CallHandler:
			public GThread,
			public X::XProxy,
			public RemotingProc
		{
			XWait mStartReadWait;
			XWait mWriteWait;
			XWait mCanReadWait;
			XWait mFinishReadWait;
		protected:
			CallCounter mCallCounter;
			// uses for write data
			SMSwapBuffer* mWBuffer = nullptr;
			SwapBufferStream mWStream;
			// uses for read data
			SMSwapBuffer* mRBuffer = nullptr;
			SwapBufferStream mRStream;

			// Vector to manage request IDs
			std::vector<unsigned int> mRequestVector;
			unsigned int mNextRequestId = 1;
			unsigned int mMinReqId = -1;//need to config into derived class
			unsigned int mMaxReqId = -1;
			std::mutex mCallMutex;
			std::atomic<bool> m_running{ true };
			unsigned int mCurrentCallIndex =0;

			std::atomic<int> mRefCount{ 0 };
		protected:
			std::mutex mRemoteObjectMutex;
			std::vector<X::XObj*> mRemoteObjects;
			virtual void AddObject(XObj* obj) override
			{
				mRemoteObjectMutex.lock();
				mRemoteObjects.push_back(obj);
				mRemoteObjectMutex.unlock();
			}
			virtual void RemoveOject(XObj* obj) override
			{
				mRemoteObjectMutex.lock();
				mRemoteObjects.erase(std::remove(mRemoteObjects.begin(), 
					mRemoteObjects.end(), obj), mRemoteObjects.end());
				mRemoteObjectMutex.unlock();
			}
			//the another side if exits, 
			//have to release these objects

			void CleanRemoteObjects()
			{
				mRemoteObjectMutex.lock();
				for (auto pObj : mRemoteObjects)
				{
					RemoteObject* pRC = dynamic_cast<RemoteObject*>(pObj);
					if (pRC)
					{
						pRC->SetProxy(nullptr);//remove from proxy
					}
				}
				mRemoteObjects.clear();
				mRemoteObjectMutex.unlock();
			}
		public:
			void StartReadThread()
			{
				mStartReadWait.Release(true);
			}
			//override RemotingProc's virtual functions
			inline virtual int AddRef() override
			{
				//fetch_add will return the value before add, so need to +1
				return mRefCount.fetch_add(1, std::memory_order_relaxed)+1;
			}
			inline virtual int Release() override
			{
				//fetch_sub will return the value before sub, so need to -1
				int refCount = mRefCount.fetch_sub(1, std::memory_order_acq_rel)-1;
				if (refCount == 0)
				{
					delete this;
				}
				return refCount;
			}
			inline virtual int RefCount() override
			{
				return mRefCount.load(std::memory_order_relaxed);
			}
			
			virtual void EndReceiveCall(SwapBufferStream& stream) override
			{
				mRBuffer->EndRead();
				mFinishReadWait.Release(true);
			}
			virtual SwapBufferStream& BeginWriteReturn(void* pCallContext,long long retCode) override
			{
				AutoCallCounter autoCounter(mCallCounter);
				//write my request then wait it in front of queue
				//we use another side's call index as my request id
				//because another side is not in my range
				Call_Context* pContext = (Call_Context*)pCallContext;
				unsigned int reqId = pContext->reqId;
				{
					std::unique_lock<std::mutex> lock(mCallMutex);
					mRequestVector.push_back(reqId);
				}
				//Wait this Request Queue's front is my request
				mWriteWait.Wait(-1, [this, reqId]() {
					std::unique_lock<std::mutex> lock(mCallMutex);
					return !m_running || mRequestVector.front() == reqId;
					});
				if (m_running)
				{
					//then safe to write
					mWBuffer->BeginWrite();
					mWStream.ReInit();
					mWStream.SetSMSwapBuffer(mWBuffer);
					PayloadFrameHead& head = mWBuffer->GetHead();
					head.payloadType = PayloadType::Send;
					head.size = 0; // update later
					head.callType = 0;//TODO: check this
					{
						std::unique_lock<std::mutex> lock(mCallMutex);
						head.callIndex = mCurrentCallIndex;
					}
					head.callReturnCode = retCode;
				}
				return mWStream;
			}
			virtual void EndWriteReturn(void* pCallContext, long long retCode) override
			{
				AutoCallCounter autoCounter(mCallCounter);

				//Write back the original call index/ReqId
				Call_Context* pContext = (Call_Context*)pCallContext;
				PayloadFrameHead& head = mWBuffer->GetHead();
				head.callIndex = pContext->reqId;
				//Deliver the last block
				head.payloadType = PayloadType::SendLast;
				head.size = mWStream.Size();
				//use SwapBuffer is shared memory buffer,
				//we assume it is not too big more then 2G
				//so keep as one block with blockSize
				head.blockSize = (unsigned int)mWStream.GetPos().offset;

				mWBuffer->EndWrite();
				unsigned int reqId = pContext->reqId;
				//remove my request from queue
				{
					std::unique_lock<std::mutex> lock(mCallMutex);
					assert(mRequestVector.front() == reqId);
					mRequestVector.erase(mRequestVector.begin());
				}
				//Notify all waits to check if it is their turn
				mWriteWait.Release(true);
			}
		protected:
			SwapBufferStream& BeginCall(unsigned int callType, Call_Context& context)
			{
				AutoCallCounter autoCounter(mCallCounter);

				unsigned int reqId = 0;
				{
					std::unique_lock<std::mutex> lock(mCallMutex);
					reqId = mNextRequestId++;
					if (mNextRequestId > mMaxReqId)
					{
						mNextRequestId = mMinReqId;
					}
					context.reqId = reqId;
					mRequestVector.push_back(reqId);
				}
				//Wait this Request Queue's front is my request
				mWriteWait.Wait(-1, [this, reqId]() {
					std::unique_lock<std::mutex> lock(mCallMutex);
					return !m_running || mRequestVector.front() == reqId;
				});
				//then safe to write
				if (m_running)
				{
					mWStream.ReInit();
					mWBuffer->BeginWrite();
					mWStream.SetSMSwapBuffer(mWBuffer);
					PayloadFrameHead& head = mWBuffer->GetHead();
					head.payloadType = PayloadType::Send;
					head.size = 0; // update later
					head.callType = callType;
					head.callIndex = reqId;
					head.callReturnCode = 0;
				}

				return mWStream;
			}

			SwapBufferStream& CommitCall(Call_Context& context,long long& returnCode)
			{
				AutoCallCounter autoCounter(mCallCounter);

				PayloadFrameHead& head = mWBuffer->GetHead();
				//Deliver the last block
				head.payloadType = PayloadType::SendLast;
				head.size = mWStream.Size();
				//use SwapBuffer is shared memory buffer,
				//we assume it is not too big more then 2G
				//so keep as one block with blockSize
				head.blockSize = (unsigned int)mWStream.GetPos().offset;
				mWBuffer->EndWrite();//Notify another side
				//remove my request from queue
				{
					std::unique_lock<std::mutex> lock(mCallMutex);
					assert(mRequestVector.front() == context.reqId);
					mRequestVector.erase(mRequestVector.begin());
				}
				//Notify all waits to check if it is their turn
				mWriteWait.Release(true);

				unsigned int rec_CallIndex = 0;
				unsigned int reqId = context.reqId;
				mCanReadWait.Wait(-1, [&]() {
					std::unique_lock<std::mutex> lock(mCallMutex);
					rec_CallIndex = mCurrentCallIndex;
					return (!m_running) || (rec_CallIndex == reqId);
					});
				//Then fetch Result
				mRStream.ReInit();
				mRStream.SetSMSwapBuffer(mRBuffer);
				mRStream.Refresh();
				PayloadFrameHead& head_back = mRBuffer->GetHead();
				returnCode = head_back.callReturnCode;
				if (returnCode == 0)
				{
					std::cout << "CommitCall,returnCode==0" << std::endl;
				}
				return mRStream;
			}

			void FinishCall()
			{
				AutoCallCounter autoCounter(mCallCounter);

				mRBuffer->EndRead();
				mFinishReadWait.Release(true);
			}

			virtual void run() override
			{
				AutoCallCounter autoCounter(mCallCounter);

				AddRef();
				mStartReadWait.Wait(-1);
				while (m_running)
				{
					if (!mRBuffer->BeginRead())
					{
						continue;
					}
					PayloadFrameHead& head = mRBuffer->GetHead();
					//callIndex is in my side's range, should be my call's return
					if (head.callIndex >= mMinReqId && head.callIndex <= mMaxReqId)
					{
						{
							std::unique_lock<std::mutex> lock(mCallMutex);
							mCurrentCallIndex = head.callIndex;
						}
						mCanReadWait.Release(true);
					}
					else//call from other side, callIndex is in another side's range
					{
						ReceiveCall();
					}
					//wait for read finishing before next read
					mFinishReadWait.Wait(-1);
				}
				Release();
			}

			void ReceiveCall()
			{
				AutoCallCounter autoCounter(mCallCounter);

				SwapBufferStream stream;
				stream.SetSMSwapBuffer(mRBuffer);
				stream.Refresh();
				PayloadFrameHead& head = mRBuffer->GetHead();
				Call_Context context;
				context.reqId = head.callIndex;
				RemoteFuncInfo* pFuncInfo = RemotingMethod::I().Get(head.callType);
				if (pFuncInfo != nullptr && pFuncInfo->pHandler != nullptr)
				{
					bool bOK = pFuncInfo->pHandler->Call(&context, head.callType, stream, this);
				}
				else
				{
					//wrong call, also need to call lines below
				}
			}
		public:
			CallHandler()
			{
			}

			~CallHandler()
			{
				StopRunning();
				WaitToEnd();
			}
			void Quit()
			{
				StopRunning();
				CleanRemoteObjects();
				if (mWBuffer)
				{
					mWBuffer->ReleaseEvents();
				}
				if (mRBuffer)
				{
					mRBuffer->ReleaseEvents();
				}
			}
			void Close()
			{
				if (mWBuffer)
				{
					mWBuffer->Close();
				}
				if (mRBuffer)
				{
					mRBuffer->Close();
				}
			}
			void StopRunning()
			{
				mStartReadWait.Release(true);
				{
					std::unique_lock<std::mutex> lock(mCallMutex);
					m_running = false;
				}
			}
			void ReStart()
			{
				m_running = true;
				mStartReadWait.Reset();
				Start();
			}
			//XProxy Virtual functions
			X::Value UpdateItemValue(X::ROBJ_ID parentObjId, X::ROBJ_ID id,
				Port::vector<std::string>& IdList, int id_offset,
				std::string itemName, X::Value& val)
			{
				AutoCallCounter autoCounter(mCallCounter);

				Call_Context context;
				auto& stream = BeginCall((unsigned int)RPC_CALL_TYPE::CantorProxy_UpdateItemValue, context);
				stream << parentObjId;
				stream << id;
				stream << (int)IdList.size();
				for (auto& s : IdList)
				{
					stream << s;
				}
				stream << id_offset;
				stream << itemName;
				stream << val;
				long long returnCode = 0;
				auto& stream2 = CommitCall(context, returnCode);
				X::Value retVal;
				if (returnCode > 0)
				{
					stream2 >> retVal;
				}
				FinishCall();
				return retVal;
			}

			bool FlatPack(X::ROBJ_ID parentObjId, X::ROBJ_ID id,
				Port::vector<std::string>& IdList, int id_offset,
				long long startIndex, long long count, Value& retList)
			{
				AutoCallCounter autoCounter(mCallCounter);

				Call_Context context;
				auto& stream = BeginCall((unsigned int)RPC_CALL_TYPE::CantorProxy_FlatPack, context);
				stream << parentObjId;
				stream << id;
				stream << (int)IdList.size();
				for (auto& s : IdList)
				{
					stream << s;
				}
				stream << id_offset;
				stream << startIndex;
				stream << count;
				long long returnCode = 0;
				auto& stream2 = CommitCall(context, returnCode);
				if (returnCode > 0)
				{
					stream2 >> retList;
				}
				FinishCall();
				return true;
			}

			X::ROBJ_MEMBER_ID QueryMember(X::ROBJ_ID id, std::string& name,
				int& memberFlags)
			{
				AutoCallCounter autoCounter(mCallCounter);

				X::ROBJ_MEMBER_ID mId = -1;
				Call_Context context;
				auto& stream = BeginCall((unsigned int)RPC_CALL_TYPE::CantorProxy_QueryMember, context);
				stream << id;
				stream << name;
				long long returnCode = 0;
				auto& stream2 = CommitCall(context, returnCode);
				if (returnCode > 0)
				{
					stream2 >> mId;
					stream2 >> memberFlags;
				}
				FinishCall();
				return mId;
			}
			long long QueryMemberCount(X::ROBJ_ID id)
			{
				AutoCallCounter autoCounter(mCallCounter);

				Call_Context context;
				auto& stream = BeginCall((unsigned int)RPC_CALL_TYPE::CantorProxy_QueryMemberCount, context);
				stream << id;
				long long returnCode = 0;
				auto& stream2 = CommitCall(context, returnCode);
				long long cnt = -1;
				if (returnCode > 0)
				{
					stream2 >> cnt;
				}
				FinishCall();
				return cnt;
			}
			bool ReleaseObject(ROBJ_ID id)
			{
				if(!m_running)
				{
					return false;
				}
				AutoCallCounter autoCounter(mCallCounter);

				Call_Context context;
				auto& stream = BeginCall((unsigned int)RPC_CALL_TYPE::CantorProxy_ReleaseObject, context);
				stream << id;
				long long returnCode = 0;
				auto& stream2 = CommitCall(context, returnCode);
				FinishCall();
				return (returnCode > 0);
			}
			X::ROBJ_ID GetMemberObject(X::ROBJ_ID objid, X::ROBJ_MEMBER_ID memId)
			{
				AutoCallCounter autoCounter(mCallCounter);

				X::ROBJ_ID oId = { 0,0 };
				Call_Context context;
				auto& stream = BeginCall((unsigned int)RPC_CALL_TYPE::CantorProxy_GetMemberObject, context);
				stream << objid;
				stream << memId;
				long long returnCode = 0;
				auto& stream2 = CommitCall(context, returnCode);
				if (returnCode > 0)
				{
					stream2 >> oId;
				}
				FinishCall();
				return oId;
			}
			bool Call(XRuntime* rt, XObj* pContext,
				X::ROBJ_ID parent_id, X::ROBJ_ID id, X::ROBJ_MEMBER_ID memId,
				X::ARGS& params, X::KWARGS& kwParams, X::Value& trailer, X::Value& retValue)
			{
				AutoCallCounter autoCounter(mCallCounter);

				X::ROBJ_ID oId = { 0,0 };
				Call_Context callContext;
				auto& stream = BeginCall((unsigned int)RPC_CALL_TYPE::CantorProxy_Call, callContext);
				stream.ScopeSpace().SetContext((XlangRuntime*)rt, pContext);

				stream << parent_id;
				stream << id;
				stream << memId;

				int argNum = (int)params.size();
				stream << argNum;
				//Pack Parameters
				for (auto& param : params)
				{
					bool bNeedConvert = false;
					if (param.IsObject())
					{
						// for function as an event handler, we need to convert it to remote client object
						//converT to remote client object
						auto* pObj = param.GetObj();
						auto type = pObj->GetType();
						if (type == X::ObjType::Function || type == X::ObjType::PyProxyObject)
						{
							bNeedConvert = true;
						}
					}
					if (bNeedConvert)
					{
						auto&& rcParam = ConvertXObjToRemoteClientObject(param.GetObj());
						rcParam.ToBytes(&stream);
					}
					else
					{
						param.ToBytes(&stream);
					}
				}
				stream << (int)kwParams.size();
				for (auto& kw : kwParams)
				{
					stream << kw.key;
					bool bNeedConvert = false;
					if (kw.val.IsObject())
					{
						// for function as an event handler, we need to convert 
						// it to remote client object
						//convert to remote client object
						auto* pObj = kw.val.GetObj();
						auto type = pObj->GetType();
						if (type == X::ObjType::Function || type == X::ObjType::PyProxyObject)
						{
							bNeedConvert = true;
						}
					}
					if (bNeedConvert)
					{
						//convert to remote client object
						auto&& rcParam = ConvertXObjToRemoteClientObject(kw.val.GetObj());
						rcParam.ToBytes(&stream);
					}
					else
					{
						kw.val.ToBytes(&stream);
					}
				}
				//set flag to show if there is a trailer
				stream << trailer.IsValid();
				if (trailer.IsValid())
				{
					stream << trailer;
				}
				long long returnCode = 0;
				auto& stream2 = CommitCall(callContext, returnCode);
				if (returnCode > 0)
				{
					X::ROBJ_ID retId = { 0,0 };
					stream2 >> retId;
					if (retId.objId == 0)
					{//value
						retValue.FromBytes(&stream2);
					}
					else if (retId.pid == GetPID())
					{
						//need to use function to do convertion and check
						auto pRetObj = (X::XObj*)retId.objId;
						retValue = (X::XObj*)pRetObj;
					}
					else
					{
						X::XRemoteObject* pRetObj =
							X::g_pXHost->CreateRemoteObject(this);
						pRetObj->SetObjID((unsigned long)retId.pid, retId.objId);
						retValue = (X::XObj*)pRetObj;
						pRetObj->DecRef();
					}
				}
				FinishCall();
				return (returnCode > 0);
			}
		protected:
			X::Value ConvertXObjToRemoteClientObject(X::XObj* obj)
			{
				obj->IncRef();
				auto pid = GetPID();
				X::ROBJ_ID robjId{ pid,obj };
				RemoteObject* pRC = new RemoteObject(nullptr);
				pRC->SetObjID(pid, obj);
				X::Value val(pRC);
				return val;
			}
		};
	}
}
