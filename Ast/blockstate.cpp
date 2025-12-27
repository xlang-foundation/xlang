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

			if (incomingId == OP_ID::Equal)  // "is" operator
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

} // namespace X
