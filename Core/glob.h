#pragma once
#include "singleton.h"
#include <string>
#include <unordered_map>
#include <vector>
#include "utility.h"
#include "runtime.h"
#include <iostream>
#include "Locker.h"
#include "stackframe.h"

namespace X {
	namespace Data { class Object; }
	namespace AST { class StackFrame; }
	class OpRegistry;
	class XlangRuntime;
	class G:
		public Singleton<G>
	{
		struct ObjInfo
		{
#if XLANG_ENG_DBG
			std::vector<AST::StackFrame*> stacksOwn;
#endif
		};
		std::unordered_map<long long, XlangRuntime*> m_rtMap;//for multiple threads
		void* m_lockRTMap = nullptr;
		XlangRuntime* MakeThreadRuntime(long long curTId, XlangRuntime* rt);
		std::unordered_map<Data::Object*, ObjInfo> Objects;
		void* m_lock = nullptr;
		OpRegistry* m_reg = nullptr;
		void notityThread(const char* strType, int tid);
	public:
		G();
		~G();
		void BindRuntimeToThread(XlangRuntime* rt);
		void UnbindRuntimeToThread(XlangRuntime* rt);
		FORCE_INLINE OpRegistry& R() { return *m_reg;}
		FORCE_INLINE void SetReg(OpRegistry* r) { m_reg = r; }
		FORCE_INLINE std::unordered_map<long long, XlangRuntime*> GetThreadRuntimeIdMap() 
		{
			((Locker*)m_lockRTMap)->Lock();
			std::unordered_map<long long, XlangRuntime*> ret(m_rtMap);
			((Locker*)m_lockRTMap)->Unlock();
			return  ret; 
		}
		FORCE_INLINE AST::Module* QueryModuleByThreadId(int threadId)
		{
			AST::Module* ret = nullptr;
			((Locker*)m_lockRTMap)->Lock();
			auto it = m_rtMap.find(threadId);
			if (it != m_rtMap.end())
			{
				ret = it->second->M();
			}
			((Locker*)m_lockRTMap)->Unlock();
			return  ret;
		}

		FORCE_INLINE XRuntime* GetCurrentRuntime(XRuntime* baseRt = nullptr)
		{
			unsigned long tid = GetThreadID();
			XRuntime* pRet = nullptr;
			((Locker*)m_lockRTMap)->Lock();
			auto it = m_rtMap.find(tid);
			if (it != m_rtMap.end())
			{
				pRet = dynamic_cast<XRuntime*>(it->second);
			}
			((Locker*)m_lockRTMap)->Unlock();
			if (pRet == nullptr)
			{
				pRet =  MakeThreadRuntime(tid, baseRt?dynamic_cast<XlangRuntime*>(baseRt):nullptr);
			}
			return pRet;
		}
		FORCE_INLINE XlangRuntime* Threading(XlangRuntime* fromRt)
		{
			long long curTId = GetThreadID();
			if (fromRt == nullptr || fromRt->GetThreadId() != curTId)
			{
				fromRt = MakeThreadRuntime(curTId, fromRt);
			}
			return fromRt;
		}

		void Lock();
		void UnLock();
		void Check()
		{
			Lock();
			auto size = Objects.size();
			UnLock();
			//std::cout << "Left Objects:" << size << std::endl;
		}
#if XLANG_ENG_DBG
		void ObjBindToStack(XObj* pXObj, AST::StackFrame* pStack);
		void ObjUnbindToStack(XObj* pXObj, AST::StackFrame* pStack);
#endif
		FORCE_INLINE void AddObj(Data::Object* obj)
		{
			Lock();
			Objects.emplace(std::make_pair(obj,ObjInfo{}));
			UnLock();
		}
		FORCE_INLINE void RemoveObj(Data::Object* obj)
		{
			Lock();
			auto it = Objects.find(obj);
			if (it != Objects.end())
			{
#if XLANG_ENG_DBG
				if (it->second.stacksOwn.size() > 0)
				{
					std::cout << "Object still have ref in stack" 
						<< it->second.stacksOwn.size() << std::endl;
				}
#endif
				Objects.erase(it);
			}
			UnLock();
		}
	};
}