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
			const unsigned int mMinReqId = 1;
			const unsigned int mMaxReqId = 1000;
			std::mutex mCallMutex;
			std::condition_variable mWriteConditionVar;
			std::condition_variable mReadConditionVar;
			std::condition_variable mFinishConditionVar;
			std::atomic<bool> m_running{ true };
			std::thread mReadThread;
			std::atomic<unsigned int> mCurrentCallIndex{ 0 };

			std::atomic<int> mRefCount{ 0 };
		public:
			//override RemotingProc's virtual functions
			virtual int AddRef() override
			{
				return mRefCount.fetch_add(1, std::memory_order_relaxed) + 1;
			}
			virtual int Release() override
			{
				if (mRefCount.fetch_sub(1, std::memory_order_acq_rel) == 1)
				{
					delete this;
				}
			}
			virtual void EndReceiveCall(SwapBufferStream& stream) override
			{
				mRBuffer->EndRead();
			}
			virtual void BeginWriteReturn(SwapBufferStream& stream, bool callIsOk) override
			{
				mWBuffer->BeginWrite();
				mWStream.SetSMSwapBuffer(mSMSwapBuffer1);
				PayloadFrameHead& head = mWBuffer->GetHead();
				head.payloadType = PayloadType::Send;
				head.size = 0; // update later
				head.callType = PayloadType::Return;
				head.callIndex = mCurrentCallIndex.load();
				head.context = nullptr;
			}
			virtual void EndWriteReturn(void* pCallContext, SwapBufferStream& stream, bool callIsOk) override
			{
				mWBuffer->EndWrite();
				mRBuffer->BeginRead();
				PayloadFrameHead& head = mRBuffer->GetHead();
				if (head.callIndex == mCurrentCallIndex.load())
				{
					mRBuffer->EndRead();
					mFinishConditionVar.notify_all();
				}
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
					mWriteConditionVar.wait(lock, [&] { return !m_running
						|| mRequestVector.front() == reqId; });
				}
				if (m_running)
				{
					mWStream.ReInit();
					mWBuffer->BeginWrite();
					mWStream.SetSMSwapBuffer(mSMSwapBuffer1);
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
				mWBuffer->EndWrite();

				{
					std::unique_lock<std::mutex> lock(mCallMutex);
					assert(mRequestVector.front() == context.reqId);
					mRequestVector.erase(mRequestVector.begin());
					mWriteConditionVar.notify_all();
				}

				{
					std::unique_lock<std::mutex> lock(mCallMutex);
					mReadConditionVar.wait(lock, [&] { return !m_running
						|| mCurrentCallIndex == context.reqId; });
				}
				//Fetch Result
				mRStream.ReInit();
				mRStream.SetSMSwapBuffer(mRBuffer);
				mRStream.Refresh();

				return mRStream;
			}

			void FinishCall()
			{
				mRBuffer->EndRead();
				{
					std::unique_lock<std::mutex> lock(mCallMutex);
					mFinishConditionVar.notify_all();
				}
			}

			void ReadThread()
			{
				// make first BeginRead will be called
				{
					std::unique_lock<std::mutex> lock(mCallMutex);
					mFinishConditionVar.notify_all();
				}

				while (m_running)
				{
					{
						std::unique_lock<std::mutex> lock(mCallMutex);
						mFinishConditionVar.wait(lock, [&] { return !m_running; });
					}
					if (!m_running) break;

					mRBuffer->BeginRead();
					PayloadFrameHead& head = mRBuffer->GetHead();
					if (head.callIndex >= mMinReqId && head.callIndex <= mMaxReqId)
					{
						mCurrentCallIndex.store(head.callIndex);
						mReadConditionVar.notify_all();
					}
					else
					{
						ReceiveCall();
					}
				}
			}

			void ReceiveCall()
			{
				SwapBufferStream stream;
				stream.SetSMSwapBuffer(mRBuffer);
				stream.Refresh();
				PayloadFrameHead& head = mRBuffer->GetHead();
				RemoteFuncInfo* pFuncInfo = RemotingMethod::I().Get(head.callType);
				if (pFuncInfo != nullptr && pFuncInfo->pHandler != nullptr)
				{
					bool bOK = pFuncInfo->pHandler->Call(head.context, head.callType, stream, this);
				}
				else
				{
					//wrong call, also need to call lines below
				}
			}

		public:
			CallHandler()
			{
				mReadThread = std::thread(&CallHandler::ReadThread, this);
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
					mWriteConditionVar.notify_all();
					mReadConditionVar.notify_all();
					mFinishConditionVar.notify_all();
				}
			}
		};
	}
}
