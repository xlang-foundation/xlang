#pragma once
#include "singleton.h"
#include <string>
#include <unordered_map>
#include <vector>
#include "def.h"

namespace X {
	namespace Data { class Object; }
	class G:
		public Singleton<G>
	{
		std::unordered_map<Data::Object*, int> Objects;
		std::vector<short> kwTree;
		std::vector<OpAction> OpActions;
		int opids[(int)OP_ID::Count]={0};
	public:
		void Check()
		{
			auto size = Objects.size();
		}
		inline void AddObj(Data::Object* obj)
		{
			Objects.emplace(std::make_pair(obj,1));
		}
		inline void RemoveObj(Data::Object* obj)
		{
			auto it = Objects.find(obj);
			if (it != Objects.end())
			{
				Objects.erase(it);
			}
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