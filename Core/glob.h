#pragma once
#include "singleton.h"
#include <string>
#include <unordered_map>
#include <vector>
#include "def.h"

namespace XPython {
	class G:
		public Singleton<G>
	{
		std::vector<short> kwTree;
		std::vector<OpAction> OpActions;
		int opids[(int)OP_ID::Count]={0};
	public:
		inline int GetOpId(OP_ID idx)
		{
			return opids[(int)idx];
		}
		inline void SetOpId(OP_ID idx, int id)
		{
			opids[(int)idx] = id;
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