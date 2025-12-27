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
	namespace AST { 
		class Block;
		class TernaryOp;
		class ListComprehension;
		class DictComprehension;
	}

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
	
	// === NEW: Track expression context for inline if/else detection ===
	bool m_inExpressionContext = false;
	int m_expressionDepth = 0;  // Track nested expression depth
	
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
	FORCE_INLINE std::stack<AST::Expression*>& Operands()
	{
		return m_operands;
	}
	FORCE_INLINE std::stack<AST::Operator*>& Ops()
	{
		return m_ops;
	}
	
	// === NEW: Expression context management ===
	FORCE_INLINE void SetExpressionContext(bool b) { m_inExpressionContext = b; }
	FORCE_INLINE bool IsExpressionContext() const { return m_inExpressionContext; }
	FORCE_INLINE void EnterExpression() { m_expressionDepth++; m_inExpressionContext = true; }
	FORCE_INLINE void LeaveExpression() 
	{ 
		if (m_expressionDepth > 0) m_expressionDepth--; 
		m_inExpressionContext = (m_expressionDepth > 0);
	}
	FORCE_INLINE int GetExpressionDepth() const { return m_expressionDepth; }
	
	// === NEW: Check if 'if' should be inline (ternary) or block ===
	// Returns true if we're in expression context (operands exist, not at line start)
	FORCE_INLINE bool ShouldBeInlineIf() const
	{
		// If there are operands on the stack and we're not at a new line start,
		// this 'if' is part of a ternary expression
		return !m_operands.empty() && !m_NewLine_WillStart;
	}
	
	// === NEW: Check if 'else' should be inline (part of ternary) ===
	bool ShouldBeInlineElse() const;
	
	// === NEW: Check if 'for' should be inline (list comprehension) ===
	// Returns true if we're inside brackets/braces (comprehension context)
	FORCE_INLINE bool ShouldBeInlineFor() const
	{
		// Check if we're inside a pair (brackets or braces)
		if (!m_stackPair.empty())
		{
			const PairInfo& top = m_stackPair.top();
			// OP_ID for '[' or '{' indicates comprehension context
			return (top.opId == OP_ID::Brackets_L || top.opId == OP_ID::Curlybracket_L);
		}
		return false;
	}
	
	// === NEW: Handle inline if token - creates InlineIfOp or block If ===
	// Declared here, implemented in blockstate.cpp
	AST::Expression* HandleIfToken(short tokenOp, int tokenIndex);
	
	// === NEW: Handle inline else token ===
	AST::Expression* HandleElseToken(short tokenOp, int tokenIndex);
	
	// === NEW: Handle inline for in comprehension context ===
	// Returns ListComprehension or DictComprehension, or nullptr for block for
	AST::Expression* HandleForToken(short tokenOp, int tokenIndex);
	
	// === NEW: Build list comprehension from parsed components ===
	AST::ListComprehension* BuildListComprehension(
		AST::Expression* outputExpr,
		AST::Expression* loopVar,
		AST::Expression* iterable,
		AST::Expression* filterCond = nullptr);
	
	// === NEW: Build dict comprehension from parsed components ===
	AST::DictComprehension* BuildDictComprehension(
		AST::Expression* keyExpr,
		AST::Expression* valueExpr,
		AST::Expression* loopVar,
		AST::Expression* iterable,
		AST::Expression* filterCond = nullptr);
	
	// === NEW: Finalize ternary expression when all parts are collected ===
	AST::TernaryOp* FinalizeTernary(
		AST::Expression* trueExpr,
		AST::Expression* condition,
		AST::Expression* falseExpr);

	// Existing methods - keep FORCE_INLINE for performance critical ones
	FORCE_INLINE void ProcessPrecedenceOp2(short lastToken,
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
	
	// Move complex implementation to cpp file
	void ProcessPrecedenceOp(short lastToken, AST::Operator* curOp);
	
	FORCE_INLINE void PushDecor(AST::Decorator* p)
	{
		m_unsolved_decors.push_back(p);
	}
	
	// Move to cpp file - has complex logic
	void HaveNewLine(AST::Expression* newLine);
	
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

} // namespace X
