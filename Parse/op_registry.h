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