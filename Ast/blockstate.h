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
#include "exp.h"
#include "op.h"
#include "glob.h"
#include "decor.h"
#include <stack>
#include "func.h"
#include "op_registry.h"
//for compile not for runtime
namespace X {
	namespace AST { class Block;}

struct PairInfo
{
	OP_ID opId = OP_ID::None;
	int opIndex;
	bool IsLambda = false;
};
class BlockState
{
	AST::Block* m_pBlock = nullptr;
	std::stack<AST::Expression*> m_operands;
	std::stack<AST::Operator*> m_ops;
	std::stack<PairInfo> m_stackPair;
	std::vector<AST::Decorator*> m_unsolved_decors;
	FORCE_INLINE OpAction OpAct(short idx)
	{
		return G::I().R().OpAct(idx);
	}
public:
	//if meet some keyword like select, will set it true to skip \n
	bool m_SkipLineFeedN = false;
	//below,before meet first non-tab char,it is true 
	bool m_NewLine_WillStart = true;
	int m_TabCountAtLineBegin = 0;
	int m_LeadingSpaceCountAtLineBegin = 0;

	BlockState(AST::Block* pBlock)
	{
		m_pBlock = pBlock;
	}
	std::stack<PairInfo>& StackPair() { return m_stackPair; }
	AST::Block* Block() { return m_pBlock; }
	FORCE_INLINE void PushExp(AST::Expression* exp)
	{
		m_operands.push(exp);
	}
	FORCE_INLINE void PushOp(AST::Operator* op)
	{
		m_ops.push(op);
	}
	FORCE_INLINE bool IsOpStackEmpty()
	{
		return m_ops.empty();
	}
	FORCE_INLINE bool IsOperandStackEmpty()
	{
		return m_operands.empty();
	}
	FORCE_INLINE void OperandPop()
	{
		m_operands.pop();
	}
	FORCE_INLINE AST::Expression* OperandTop()
	{
		return m_operands.top();
	}
	FORCE_INLINE AST::Operator* OpTop()
	{
		return m_ops.top();
	}
	FORCE_INLINE void OpPop()
	{
		m_ops.pop();
	}
	FORCE_INLINE void ProcessPrecedenceOp(short lastToken,
		AST::Operator* curOp)
	{
		while (!m_ops.empty())
		{
			auto top = m_ops.top();
			OpAction topAct = OpAct(top->getOp());
			OpAction cur_opAct = OpAct(curOp->getOp());
			//check this case .[test1,test2](....)
			//after . it is a ops,not var
			//todo: 3/20/2023, for tensor, want to process left first if same precedence
			//so change here,

			//todo: 3/23/2023 shawn,comment 'lastToken != top->getOp()' out for cause:t1[:,:2]
			//check if some other impacts
			if (/*lastToken != top->getOp()
				&& */top->m_type != AST::ObType::Pair
				//&& topAct.precedence > cur_opAct.precedence)
				&& topAct.precedence >= cur_opAct.precedence)
			{
				DoOpTop();
			}
			else
			{
				break;
			}
		}
	}
	FORCE_INLINE void PushDecor(AST::Decorator* p)
	{
		m_unsolved_decors.push_back(p);
	}
	FORCE_INLINE void HaveNewLine(AST::Expression* newLine)
	{
		if (newLine->m_type == AST::ObType::Decor)
		{
			m_unsolved_decors.push_back(dynamic_cast<AST::Decorator*>(newLine));
		}
		else if (newLine->m_type == AST::ObType::Param)
		{
			//for case: def func(): or class also
			AST::Param* pParam = dynamic_cast<AST::Param*>(newLine);
			auto* pNameOfParam = pParam->GetName();
			if (pNameOfParam && (pNameOfParam->m_type == AST::ObType::Class ||
				pNameOfParam->m_type == AST::ObType::Func))
			{
				AST::Func* pFunc = dynamic_cast<AST::Func*>(pNameOfParam);
				for (auto* d : m_unsolved_decors)
				{
					pFunc->AddDecor(d);
				}
				m_unsolved_decors.clear();
			}
		}
		else if(newLine->m_type == AST::ObType::Class ||
			newLine->m_type == AST::ObType::Func)
		{
			AST::Func* pFunc = dynamic_cast<AST::Func*>(newLine);
			for (auto* d : m_unsolved_decors)
			{
				pFunc->AddDecor(d);
			}
			m_unsolved_decors.clear();
		}
		else
		{//not func or class, clear
			m_unsolved_decors.clear();
		}
	}
	FORCE_INLINE bool DoOp(AST::Operator* op)
	{
		return op->OpWithOperands(m_operands,-1);
	}
	FORCE_INLINE bool DoOpTop()
	{
		auto top = m_ops.top();
		m_ops.pop();
		//get the next op,take its tokenIndex to constraint OpWithOperands
		int tokenIndex = -1;
		if (!m_ops.empty())
		{
			auto next = m_ops.top();
			tokenIndex = next->GetTokenIndex();
		}
		return top->OpWithOperands(m_operands, tokenIndex);
	}
};
}