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

#include "blockstate.h"
#include "block.h"
#include "inline_expr.h"
#include "var.h"

namespace X
{

//=============================================================================
// ProcessPrecedenceOp - Moved from header for cleaner organization
//=============================================================================
void BlockState::ProcessPrecedenceOp(short lastToken, AST::Operator* curOp)
{
	// Handle compound operators: "not is" and "not in"
	if (!m_ops.empty())
	{
		auto top = m_ops.top();
		if (top->GetId() == OP_ID::NotOp)
		{
			OP_ID incomingId = curOp->GetId();

			if (incomingId == OP_ID::IsEqual)  // "is" operator
			{
				// Combine "not" + "is" -> convert to NotEqual
				m_ops.pop();
				curOp->SetId(OP_ID::NotEqual);
				delete top;
			}
			else if (incomingId == OP_ID::InOp)
			{
				// Combine "not" + "in" -> set flag on InOp
				m_ops.pop();
				AST::InOp* pInOp = dynamic_cast<AST::InOp*>(curOp);
				if (pInOp)
				{
					pInOp->SetIsNot(true);
				}
				delete top;
			}
		}
	}

	// Normal precedence processing
	while (!m_ops.empty())
	{
		auto top = m_ops.top();
		OpAction topAct = OpAct(top->getOp());
		OpAction cur_opAct = OpAct(curOp->getOp());

		if (top->m_type != AST::ObType::Pair
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

//=============================================================================
// HaveNewLine - Moved from header
//=============================================================================
void BlockState::HaveNewLine(AST::Expression* newLine)
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
	else if (newLine->m_type == AST::ObType::Class ||
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
	{
		//not func or class, clear
		m_unsolved_decors.clear();
	}
}

//=============================================================================
// ShouldBeInlineElse
// Check if 'else' is part of a ternary expression
//=============================================================================
bool BlockState::ShouldBeInlineElse() const
{
	// Check if there's a pending TernaryOp or InlineIfOp on operand stack
	if (m_operands.empty())
	{
		return false;
	}
	
	// Get the top operand - if it's a partial TernaryOp, this else completes it
	AST::Expression* top = const_cast<std::stack<AST::Expression*>&>(m_operands).top();
	if (top && top->m_type == AST::ObType::TernaryOp)
	{
		// Check if it's a partial ternary (missing false expression)
		AST::TernaryOp* ternary = dynamic_cast<AST::TernaryOp*>(top);
		if (ternary && ternary->GetFalseExpr() == nullptr)
		{
			return true;
		}
	}
	
	// Also check operator stack for InlineIfOp
	if (!m_ops.empty())
	{
		AST::Operator* topOp = const_cast<std::stack<AST::Operator*>&>(m_ops).top();
		if (topOp && topOp->m_type == AST::ObType::InlineIfOp)
		{
			return true;
		}
	}
	
	// If we're not at line start and have operands, likely inline
	return !m_operands.empty() && !m_NewLine_WillStart;
}

//=============================================================================
// HandleIfToken
// Determines if 'if' is inline (ternary) or block, creates appropriate node
//=============================================================================
AST::Expression* BlockState::HandleIfToken(short tokenOp, int tokenIndex)
{
	if (ShouldBeInlineIf())
	{
		// Create inline if operator for ternary expression
		AST::InlineIfOp* inlineIf = new AST::InlineIfOp(tokenOp);
		inlineIf->SetTokenIndex(tokenIndex);
		inlineIf->SetId(OP_ID::InlineIf);
		
		// Process precedence and push onto operator stack
		ProcessPrecedenceOp(tokenOp, inlineIf);
		m_ops.push(inlineIf);
		
		return inlineIf;
	}
	else
	{
		// Return nullptr to indicate caller should create block If
		return nullptr;
	}
}

//=============================================================================
// HandleElseToken
// Determines if 'else' is inline (ternary) or block, creates appropriate node
//=============================================================================
AST::Expression* BlockState::HandleElseToken(short tokenOp, int tokenIndex)
{
	if (ShouldBeInlineElse())
	{
		// Create inline else operator for ternary expression
		AST::InlineElseOp* inlineElse = new AST::InlineElseOp(tokenOp);
		inlineElse->SetTokenIndex(tokenIndex);
		inlineElse->SetId(OP_ID::InlineElse);
		
		// Process precedence and push onto operator stack
		ProcessPrecedenceOp(tokenOp, inlineElse);
		m_ops.push(inlineElse);
		
		return inlineElse;
	}
	else
	{
		// Return nullptr to indicate caller should handle as block else/elif
		return nullptr;
	}
}

//=============================================================================
// HandleForToken
// Determines if 'for' is inside brackets (comprehension) or block
//=============================================================================
AST::Expression* BlockState::HandleForToken(short tokenOp, int tokenIndex)
{
	if (ShouldBeInlineFor())
	{
		// We're inside brackets/braces - this is a comprehension
		// The actual comprehension building happens when we close the bracket
		// For now, we need to mark that we're in comprehension mode
		
		// Create a marker operator that will help build the comprehension
		// when the closing bracket is processed
		AST::InOp* inOp = new AST::InOp(tokenOp);
		inOp->SetTokenIndex(tokenIndex);
		
		// The 'for' in comprehension is followed by 'var in iterable'
		// We can reuse InOp since it already handles 'x in y'
		
		return inOp;
	}
	else
	{
		// Return nullptr to indicate caller should create block For
		return nullptr;
	}
}

//=============================================================================
// BuildListComprehension
// Creates a ListComprehension node from parsed components
//=============================================================================
AST::ListComprehension* BlockState::BuildListComprehension(
	AST::Expression* outputExpr,
	AST::Expression* loopVar,
	AST::Expression* iterable,
	AST::Expression* filterCond)
{
	AST::ListComprehension* comp = new AST::ListComprehension(
		outputExpr, loopVar, iterable, filterCond);
	
	// Set up parent relationships and scope
	if (outputExpr)
	{
		comp->ReCalcHint(outputExpr);
	}
	if (loopVar)
	{
		comp->ReCalcHint(loopVar);
	}
	if (iterable)
	{
		comp->ReCalcHint(iterable);
	}
	if (filterCond)
	{
		comp->ReCalcHint(filterCond);
	}
	
	return comp;
}

//=============================================================================
// BuildDictComprehension
// Creates a DictComprehension node from parsed components
//=============================================================================
AST::DictComprehension* BlockState::BuildDictComprehension(
	AST::Expression* keyExpr,
	AST::Expression* valueExpr,
	AST::Expression* loopVar,
	AST::Expression* iterable,
	AST::Expression* filterCond)
{
	AST::DictComprehension* comp = new AST::DictComprehension(
		keyExpr, valueExpr, loopVar, iterable, filterCond);
	
	// Set up hints from all components
	if (keyExpr)
	{
		comp->ReCalcHint(keyExpr);
	}
	if (valueExpr)
	{
		comp->ReCalcHint(valueExpr);
	}
	if (loopVar)
	{
		comp->ReCalcHint(loopVar);
	}
	if (iterable)
	{
		comp->ReCalcHint(iterable);
	}
	if (filterCond)
	{
		comp->ReCalcHint(filterCond);
	}
	
	return comp;
}

//=============================================================================
// FinalizeTernary
// Creates a complete TernaryOp from all three parts
//=============================================================================
AST::TernaryOp* BlockState::FinalizeTernary(
	AST::Expression* trueExpr,
	AST::Expression* condition,
	AST::Expression* falseExpr)
{
	AST::TernaryOp* ternary = new AST::TernaryOp(trueExpr, condition, falseExpr);
	
	// Set up hints from components
	if (trueExpr)
	{
		ternary->ReCalcHint(trueExpr);
	}
	if (condition)
	{
		ternary->ReCalcHint(condition);
	}
	if (falseExpr)
	{
		ternary->ReCalcHint(falseExpr);
	}
	
	return ternary;
}
//=============================================================================
// HasInlineForOp
// Check if operator stack contains InlineForOp (for comprehension detection)
//=============================================================================
bool BlockState::HasInlineForOp() const
{
	// We need to scan the operator stack without modifying it
	// Create a copy to iterate through
	std::stack<AST::Operator*> tempStack = m_ops;

	while (!tempStack.empty())
	{
		AST::Operator* op = tempStack.top();
		tempStack.pop();

		if (op && op->m_type == AST::ObType::InlineForOp)
		{
			return true;
		}

		// Stop if we hit the opening bracket/brace (pair marker)
		if (op && op->m_type == AST::ObType::Pair)
		{
			break;
		}
	}

	return false;
}

//=============================================================================
// CollectAndBuildComprehension
// Collects parameters from m_ops and m_operands stacks and builds
// ListComprehension, DictComprehension, or SetComprehension
//
// All components must have TokenIndex > pairTokenIndex (inside brackets)
//
// Stack layout for [expr for var in iterable if cond]:
//   m_operands (bottom to top): [outputExpr, loopVar, InOp(with iterable), filterCond]
//   m_ops contains: Pair([), InlineForOp, InlineIfOp (optional)
//
// Note: InOp may already be in m_operands (processed with its operands)
//=============================================================================
AST::Expression* BlockState::CollectAndBuildComprehension(OP_ID pairType)
{
	AST::Expression* filterCond = nullptr;
	AST::Expression* iterable = nullptr;
	AST::Expression* loopVar = nullptr;
	AST::Expression* outputExpr = nullptr;

	//=========================================================================
	// Step 0: Find key TokenIndex values for validation
	//=========================================================================
	int pairTokenIndex = -1;      // '[' or '{'
	int inlineForTokenIndex = -1; // 'for'
	int inOpTokenIndex = -1;      // 'in'
	int filterIfTokenIndex = -1;  // 'if' after 'for' (filter, not ternary)

	// Find pair marker '[' or '{'
	{
		std::stack<AST::Operator*> tempStack = m_ops;
		while (!tempStack.empty())
		{
			AST::Operator* op = tempStack.top();
			tempStack.pop();

			if (op == nullptr) continue;

			if (op->m_type == AST::ObType::Pair)
			{
				pairTokenIndex = op->GetTokenIndex();
				break;
			}
		}
	}
	if (pairTokenIndex < 0)
	{
		return nullptr;
	}

	// Find 'for' in m_ops
	{
		std::stack<AST::Operator*> tempStack = m_ops;
		while (!tempStack.empty())
		{
			AST::Operator* op = tempStack.top();
			tempStack.pop();

			if (op == nullptr) continue;

			if (op->m_type == AST::ObType::InlineForOp)
			{
				if (op->GetTokenIndex() > pairTokenIndex)
				{
					inlineForTokenIndex = op->GetTokenIndex();
					break;
				}
			}
		}
	}

	// Validation: must have pair and for
	if (inlineForTokenIndex < 0)
	{
		return nullptr;
	}

	// Find 'in' and filter 'if' - check both m_operands and m_ops
	// InOp may be in m_operands if it has been processed with operands
	{
		std::stack<AST::Expression*> tempStack = m_operands;
		while (!tempStack.empty())
		{
			AST::Expression* exp = tempStack.top();
			tempStack.pop();

			if (exp == nullptr) continue;

			// Check if this expression is an InOp
			if (exp->m_type == AST::ObType::In)
			{
				AST::InOp* inOp = dynamic_cast<AST::InOp*>(exp);
				if (inOp && inOp->GetTokenIndex() > inlineForTokenIndex)
				{
					inOpTokenIndex = inOp->GetTokenIndex();
				}
			}
			// Check if this is InlineIfOp (filter if)
			else if (exp->m_type == AST::ObType::InlineIfOp)
			{
				AST::Operator* op = dynamic_cast<AST::Operator*>(exp);
				if (op && op->GetTokenIndex() > inlineForTokenIndex)
				{
					filterIfTokenIndex = op->GetTokenIndex();
				}
			}
		}
	}

	// Also check m_ops for 'in' and filter 'if'
	{
		std::stack<AST::Operator*> tempStack = m_ops;
		while (!tempStack.empty())
		{
			AST::Operator* op = tempStack.top();
			tempStack.pop();

			if (op == nullptr) continue;

			if (inOpTokenIndex == -1 && op->GetId() == OP_ID::InOp)
			{
				if (op->GetTokenIndex() > inlineForTokenIndex)
				{
					inOpTokenIndex = op->GetTokenIndex();
				}
			}
			else if (filterIfTokenIndex == -1 && op->m_type == AST::ObType::InlineIfOp)
			{
				if (op->GetTokenIndex() > inlineForTokenIndex)
				{
					filterIfTokenIndex = op->GetTokenIndex();
				}
			}
		}
	}

	//=========================================================================
	// Step 1: Pop filter condition if exists
	// Filter if = InlineIfOp with TokenIndex > inlineForTokenIndex
	//=========================================================================
	if (filterIfTokenIndex > 0)
	{
		// First check if InlineIfOp is in m_ops
		while (!m_ops.empty())
		{
			AST::Operator* top = m_ops.top();

			if (top == nullptr)
			{
				m_ops.pop();
				continue;
			}

			// Stop at pair marker
			if (top->m_type == AST::ObType::Pair)
			{
				break;
			}

			// Found filter InlineIfOp in ops
			if (top->m_type == AST::ObType::InlineIfOp &&
				top->GetTokenIndex() == filterIfTokenIndex)
			{
				m_ops.pop();
				delete top;

				// Pop filter condition - must be after filter if
				if (!m_operands.empty())
				{
					AST::Expression* cond = m_operands.top();
					if (cond && cond->GetTokenIndex() > filterIfTokenIndex)
					{
						filterCond = cond;
						m_operands.pop();
					}
				}
				break;
			}

			// Process other operators
			DoOpTop();
		}

		// If filterCond is still null, check if InlineIfOp is in m_operands
		if (filterCond == nullptr)
		{
			// Need to find and extract from operands
			std::stack<AST::Expression*> tempStack;
			while (!m_operands.empty())
			{
				AST::Expression* exp = m_operands.top();
				m_operands.pop();

				if (exp && exp->m_type == AST::ObType::InlineIfOp &&
					exp->GetTokenIndex() == filterIfTokenIndex)
				{
					// Found the filter if in operands - extract condition from it
					// The filter condition should be the next operand
					if (!m_operands.empty())
					{
						AST::Expression* cond = m_operands.top();
						if (cond && cond->GetTokenIndex() > filterIfTokenIndex)
						{
							filterCond = cond;
							m_operands.pop();
						}
					}
					delete exp;  // Delete the InlineIfOp
					break;
				}
				else
				{
					tempStack.push(exp);
				}
			}
			// Restore operands
			while (!tempStack.empty())
			{
				m_operands.push(tempStack.top());
				tempStack.pop();
			}
		}
	}

	//=========================================================================
	// Step 2: Pop iterable (expression after 'in')
	// InOp may be in m_ops or m_operands
	//=========================================================================

	// First try to find InOp in m_ops
	bool foundInOpInOps = false;
	while (!m_ops.empty())
	{
		AST::Operator* top = m_ops.top();

		if (top == nullptr)
		{
			m_ops.pop();
			continue;
		}

		// Stop at pair marker
		if (top->m_type == AST::ObType::Pair)
		{
			break;
		}

		// Found InOp in ops
		if (top->GetId() == OP_ID::InOp &&
			top->GetTokenIndex() > inlineForTokenIndex)
		{
			m_ops.pop();
			foundInOpInOps = true;

			// Pop iterable - must be after 'in'
			if (!m_operands.empty())
			{
				AST::Expression* iter = m_operands.top();
				if (iter && iter->GetTokenIndex() > inOpTokenIndex)
				{
					iterable = iter;
					m_operands.pop();
				}
				else if (iter && iter->GetTokenIndex() > inlineForTokenIndex)
				{
					// Fallback: at least after 'for'
					iterable = iter;
					m_operands.pop();
				}
			}
			delete top;
			break;
		}

		// Process other operators
		DoOpTop();
	}

	// If InOp not in m_ops, check m_operands
	if (!foundInOpInOps && inOpTokenIndex > 0)
	{
		std::stack<AST::Expression*> tempStack;
		while (!m_operands.empty())
		{
			AST::Expression* exp = m_operands.top();
			m_operands.pop();

			if (exp && exp->m_type == AST::ObType::In &&
				exp->GetTokenIndex() == inOpTokenIndex)
			{
				// Found InOp in operands - it should contain the iterable as its right operand
				AST::InOp* inOp = dynamic_cast<AST::InOp*>(exp);
				if (inOp)
				{
					// Get iterable from InOp's right side
					iterable = inOp->GetR();
					// Get loop variable from InOp's left side
					loopVar = inOp->GetL();

					// Detach so they won't be deleted
					inOp->SetL(nullptr);
					inOp->SetR(nullptr);
				}
				delete exp;
				break;
			}
			else
			{
				tempStack.push(exp);
			}
		}
		// Restore operands
		while (!tempStack.empty())
		{
			m_operands.push(tempStack.top());
			tempStack.pop();
		}
	}

	//=========================================================================
	// Step 3: Pop loop variable (expression after 'for', before 'in')
	// Skip if already extracted from InOp in Step 2
	//=========================================================================
	if (loopVar == nullptr)
	{
		while (!m_ops.empty())
		{
			AST::Operator* top = m_ops.top();

			if (top == nullptr)
			{
				m_ops.pop();
				continue;
			}

			// Stop at pair marker
			if (top->m_type == AST::ObType::Pair)
			{
				break;
			}

			// Found InlineForOp
			if (top->m_type == AST::ObType::InlineForOp &&
				top->GetTokenIndex() == inlineForTokenIndex)
			{
				m_ops.pop();
				delete top;

				// Pop loop variable - must be after 'for' and before 'in'
				if (!m_operands.empty())
				{
					AST::Expression* var = m_operands.top();
					if (var && var->GetTokenIndex() > inlineForTokenIndex)
					{
						if (inOpTokenIndex < 0 || var->GetTokenIndex() < inOpTokenIndex)
						{
							loopVar = var;
							m_operands.pop();
						}
					}
				}
				break;
			}

			// Process other operators
			DoOpTop();
		}
	}
	else
	{
		// loopVar already extracted, just remove InlineForOp from ops
		while (!m_ops.empty())
		{
			AST::Operator* top = m_ops.top();

			if (top == nullptr)
			{
				m_ops.pop();
				continue;
			}

			if (top->m_type == AST::ObType::Pair)
			{
				break;
			}

			if (top->m_type == AST::ObType::InlineForOp &&
				top->GetTokenIndex() == inlineForTokenIndex)
			{
				m_ops.pop();
				delete top;
				break;
			}

			DoOpTop();
		}
	}

	//=========================================================================
	// Step 4: Pop output expression (expression after '[' or '{', before 'for')
	// Must have TokenIndex > pairTokenIndex and < inlineForTokenIndex
	//=========================================================================

	// Process any remaining operators before the pair marker
	while (!m_ops.empty())
	{
		AST::Operator* top = m_ops.top();
		if (top == nullptr)
		{
			m_ops.pop();
			continue;
		}

		// Stop at pair marker
		if (top->m_type == AST::ObType::Pair)
		{
			break;
		}

		// Process other operators
		DoOpTop();
	}

	// Pop output expression - must be inside brackets and before 'for'
	if (!m_operands.empty())
	{
		AST::Expression* expr = m_operands.top();
		if (expr &&
			expr->GetTokenIndex() > pairTokenIndex &&
			expr->GetTokenIndex() < inlineForTokenIndex)
		{
			outputExpr = expr;
			m_operands.pop();
		}
	}

	//=========================================================================
	// Step 5: Build the appropriate comprehension type
	//=========================================================================
	AST::Expression* result = nullptr;

	if (pairType == OP_ID::Brackets_L)
	{
		// List comprehension: [expr for var in iterable if cond]
		result = BuildListComprehension(outputExpr, loopVar, iterable, filterCond);
	}
	else if (pairType == OP_ID::Curlybracket_L)
	{
		// Check if outputExpr is a key:value pair (dict) or single expr (set)
		if (outputExpr && outputExpr->m_type == AST::ObType::Pair)
		{
			// Dict comprehension: {key:val for var in iterable if cond}
			AST::PairOp* pairOp = dynamic_cast<AST::PairOp*>(outputExpr);
			if (pairOp)
			{
				AST::Expression* keyExpr = pairOp->GetL();
				AST::Expression* valueExpr = pairOp->GetR();

				// Detach from pairOp so they won't be deleted
				pairOp->SetL(nullptr);
				pairOp->SetR(nullptr);
				delete pairOp;

				result = BuildDictComprehension(keyExpr, valueExpr, loopVar, iterable, filterCond);
			}
		}
		else
		{
			// Set comprehension: {expr for var in iterable if cond}
			AST::ListComprehension* setComp = BuildListComprehension(
				outputExpr, loopVar, iterable, filterCond);
			if (setComp)
			{
				setComp->SetIsSet(true);  // Mark as set comprehension
			}
			result = setComp;
		}
	}

	return result;
}

} // namespace X
