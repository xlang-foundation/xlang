#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include "def.h"
#include "utility.h"
#include "runtime.h"
#include <iostream>

namespace X 
{
	class OpRegistry
	{
		std::vector<short> kwTree;
		std::vector<OpAction> OpActions;
		int opids[(int)OP_ID::Count]={0};
	public:
		OpRegistry()
		{
		}
		~OpRegistry()
		{
		}
		FORCE_INLINE int GetOpId(OP_ID idx)
		{
			return opids[(int)idx];
		}
		FORCE_INLINE void SetOpId(OP_ID idx, int id)
		{
			opids[(int)idx] = id;
		}
		FORCE_INLINE void SetActionWithOpId()
		{
			for (int i = 0; i < (int)OP_ID::Count; i++)
			{
				OpActions[opids[i]].opId = (OP_ID)i;
			}
		}
		FORCE_INLINE std::vector<short>& GetKwTree()
		{
			return kwTree;
		}
		FORCE_INLINE std::vector<OpAction>& GetOpActions()
		{
			return OpActions;
		}
		FORCE_INLINE OpAction OpAct(short idx)
		{
			return (idx >= 0 && idx < (short)OpActions.size()) ?
				OpActions[idx] : OpAction();
		}
	};
}