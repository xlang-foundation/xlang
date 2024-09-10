#pragma once
#include "SwapBufferStream.h"
#include "SMSwapBuffer.h"
#include <vector>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <cassert>
#include "RemotingMethod.h"
#include "IpcBase.h"
#include "wait.h"

namespace X
{
	namespace IPC
	{
		struct Call_Context
		{
			unsigned int reqId = 0;
		};

		class CallHandler: public RemotingProc
		{
			XWait mWriteWait;
			XWait mCanReadWait;
			XWait mFinishReadWait;
		protected:
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
			std::thread mReadThread;
			std::atomic<unsigned int> mCurrentCallIndex{ 0 };

			std::atomic<int> mRefCount{ 0 };
		public:
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
			virtual SwapBufferStream& BeginWriteReturn(bool callIsOk) override
			{
				mWBuffer->BeginWrite();
				mWStream.SetSMSwapBuffer(mWBuffer);
				PayloadFrameHead& head = mWBuffer->GetHead();
				head.payloadType = PayloadType::Send;
				head.size = 0; // update later
				head.callType = 0;//TODO: check this
				head.callIndex = mCurrentCallIndex.load();
				head.context = nullptr;
				return mWStream;
			}
			virtual void EndWriteReturn(void* pCallContext,bool callIsOk) override
			{
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
			}
		protected:
			SwapBufferStream& BeginCall(unsigned int callType, Call_Context& context)
			{
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
					head.context = &context;
				}

				return mWStream;
			}

			SwapBufferStream& CommitCall(Call_Context& context)
			{
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

				//Wait for ready to read from another side's call return's write
				mCanReadWait.Wait(-1, [&]() {
					std::unique_lock<std::mutex> lock(mCallMutex);
					return !m_running || mCurrentCallIndex == context.reqId;
					});
				//Then fetch Result
				mRStream.ReInit();
				mRStream.SetSMSwapBuffer(mRBuffer);
				mRStream.Refresh();

				return mRStream;
			}

			void FinishCall()
			{
				mRBuffer->EndRead();
				mFinishReadWait.Release(true);
			}

			void ReadThread()
			{
				while (m_running)
				{
					if (!m_running) break;

					mRBuffer->BeginRead();
					PayloadFrameHead& head = mRBuffer->GetHead();
					//callIndex is in my side's range, should be my call's return
					if (head.callIndex >= mMinReqId && head.callIndex <= mMaxReqId)
					{
						mCurrentCallIndex.store(head.callIndex);
						mCanReadWait.Release(true);
					}
					else//call from other side, callIndex is in another side's range
					{
						ReceiveCall();
						//wait for read finishing before next read
						mFinishReadWait.Wait(-1);
					}
				}
			}

			void ReceiveCall()
			{
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
		protected:
			void StartReadThread()
			{
				mReadThread = std::thread(&CallHandler::ReadThread, this);
			}
		public:
			CallHandler()
			{
			}

			~CallHandler()
			{
				StopRunning();
				if (mReadThread.joinable())
				{
					mReadThread.join();
				}
			}

			void StopRunning()
			{
				{
					std::unique_lock<std::mutex> lock(mCallMutex);
					m_running = false;
				}
			}
		};
	}
}
