#pragma once
#include "singleton.h"
#include <string>
#include <unordered_map>
#include <vector>
#include "def.h"
#include "utility.h"
#include "runtime.h"
#include <iostream>

namespace X {
	namespace Data { class Object; }
	class Runtime;
	class G:
		public Singleton<G>
	{
		std::unordered_map<long long, Runtime*> m_rtMap;//for multiple threads
		void* m_lockRTMap = nullptr;
		Runtime* MakeThreadRuntime(long long curTId, Runtime* rt);
		std::unordered_map<Data::Object*, int> Objects;
		std::vector<short> kwTree;
		std::vector<OpAction> OpActions;
		int opids[(int)OP_ID::Count]={0};
		void* m_lock = nullptr;
	public:
		G();
		~G();
		inline Runtime* Threading(Runtime* fromRt)
		{
			long long curTId = GetThreadID();
			if (fromRt->GetThreadId() != curTId)
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
		inline int GetOpId(OP_ID idx)
		{
			return opids[(int)idx];
		}
		inline void SetOpId(OP_ID idx, int id)
		{
			opids[(int)idx] = id;
		}
		inline void SetActionWithOpId()
		{
			for (int i = 0; i < (int)OP_ID::Count; i++)
			{
				OpActions[opids[i]].opId = (OP_ID)i;
			}
		}
		inline std::vector<short>& GetKwTree()
		{
			return kwTree;
		}
		inline std::vector<OpAction>& GetOpActions()
		{
			return OpActions;
		}
		inline OpAction OpAct(short idx)
		{
			return (idx >= 0 && idx < (short)OpActions.size()) ?
				OpActions[idx] : OpAction();
		}
	};
}