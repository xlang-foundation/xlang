/*
Copyright (C) 2024 The XLang Foundation
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

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