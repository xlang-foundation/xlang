#pragma once
#include "singleton.h"
#include <string>
#include <unordered_map>
#include <vector>
#include "utility.h"
#include "runtime.h"
#include <iostream>
#include "Locker.h"

namespace X {
	namespace Data { class Object; }
	class OpRegistry;
	class Runtime;
	class G:
		public Singleton<G>
	{
		std::unordered_map<long long, Runtime*> m_rtMap;//for multiple threads
		void* m_lockRTMap = nullptr;
		Runtime* MakeThreadRuntime(long long curTId, Runtime* rt);
		std::unordered_map<Data::Object*, int> Objects;
		void* m_lock = nullptr;
		OpRegistry* m_reg = nullptr;
	public:
		G();
		~G();
		inline OpRegistry& R() { return *m_reg;}
		inline void SetReg(OpRegistry* r) { m_reg = r; }
		inline XRuntime* GetCurrentRuntime()
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
			return pRet;
		}
		inline Runtime* Threading(Runtime* fromRt)
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
			std::cout << "Left Objects:" << size << std::endl;
		}
		inline void AddObj(Data::Object* obj)
		{
			Lock();
			Objects.emplace(std::make_pair(obj,1));
			UnLock();
		}
		inline void RemoveObj(Data::Object* obj)
		{
			Lock();
			auto it = Objects.find(obj);
			if (it != Objects.end())
			{
				Objects.erase(it);
			}
			UnLock();
		}
	};
}