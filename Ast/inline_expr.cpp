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

#include "inline_expr.h"
#include "object.h"
#include "list.h"
#include "dict.h"
#include "iterator.h"
#include "exp_exec.h"

namespace X
{
    namespace AST
    {

        //=============================================================================
        // TernaryOp::Exec
        // Evaluates: value_if_true if condition else value_if_false
        //=============================================================================
        bool TernaryOp::Exec(XlangRuntime* rt, ExecAction& action, XObj* pContext,
            Value& v, LValue* lValue)
        {
            if (m_condition == nullptr || m_trueExpr == nullptr || m_falseExpr == nullptr)
            {
                return false;
            }

            // First evaluate the condition
            Value condValue;
            ExecAction condAction;
            bool bOK = ExpExec(m_condition, rt, condAction, pContext, condValue);
            if (!bOK)
            {
                return false;
            }

            // Based on condition, evaluate either true or false expression
            ExecAction exprAction;
            if (condValue.IsTrue())
            {
                bOK = ExpExec(m_trueExpr, rt, exprAction, pContext, v, lValue);
            }
            else
            {
                bOK = ExpExec(m_falseExpr, rt, exprAction, pContext, v, lValue);
            }

            return bOK;
        }

        //=============================================================================
        // ListComprehension::Exec
        // Evaluates: [expr for var in iterable] or [expr for var in iterable if cond]
        //=============================================================================
        bool ListComprehension::Exec(XlangRuntime* rt, ExecAction& action, XObj* pContext,
            Value& v, LValue* lValue)
        {
            if (m_outputExpr == nullptr || m_loopVar == nullptr || m_iterable == nullptr)
            {
                return false;
            }

            // Evaluate the iterable
            ExecAction iterAction;
            X::Value iterableObj;
            bool bOK = ExpExec(m_iterable, rt, iterAction, pContext, iterableObj);
            if (!bOK)
            {
                return false;
            }

            // Get the data object for iteration
            auto* pDataObj = dynamic_cast<Data::Object*>(iterableObj.GetObj());
            if (!pDataObj)
            {
                return false;
            }

            // Create output list
            X::Data::List* pOutList = new X::Data::List();

            // Iterate through the iterable
            X::Data::Iterator_Pos curPos = nullptr;
            std::vector<Value> vals;

            while (pDataObj->GetAndUpdatePos(curPos, vals, false))
            {
                // Assign loop variable(s)
                m_loopVar->SetArry(dynamic_cast<XlangRuntime*>(rt), pContext, vals);
                vals.clear();

                // If there's a filter condition, check it
                if (m_filterCond)
                {
                    Value filterValue;
                    ExecAction filterAction;
                    bOK = ExpExec(m_filterCond, rt, filterAction, pContext, filterValue);
                    if (!bOK)
                    {
                        delete pOutList;
                        return false;
                    }
                    // Skip this iteration if filter is false
                    if (!filterValue.IsTrue())
                    {
                        continue;
                    }
                }

                // Evaluate the output expression
                Value outValue;
                ExecAction outAction;
                bOK = ExpExec(m_outputExpr, rt, outAction, pContext, outValue);
                if (bOK)
                {
                    pOutList->Add(rt, outValue);
                }
            }

            v = X::Value(pOutList);
            return true;
        }

        //=============================================================================
        // DictComprehension::Exec
        // Evaluates: {key: value for var in iterable} or with if clause
        //=============================================================================
        bool DictComprehension::Exec(XlangRuntime* rt, ExecAction& action, XObj* pContext,
            Value& v, LValue* lValue)
        {
            if (m_keyExpr == nullptr || m_valueExpr == nullptr ||
                m_loopVar == nullptr || m_iterable == nullptr)
            {
                return false;
            }

            // Evaluate the iterable
            ExecAction iterAction;
            X::Value iterableObj;
            bool bOK = ExpExec(m_iterable, rt, iterAction, pContext, iterableObj);
            if (!bOK)
            {
                return false;
            }

            // Get the data object for iteration
            auto* pDataObj = dynamic_cast<Data::Object*>(iterableObj.GetObj());
            if (!pDataObj)
            {
                return false;
            }

            // Create output dict
            X::Data::Dict* pOutDict = new X::Data::Dict();

            // Iterate through the iterable
            X::Data::Iterator_Pos curPos = nullptr;
            std::vector<Value> vals;

            while (pDataObj->GetAndUpdatePos(curPos, vals, false))
            {
                // Assign loop variable(s)
                m_loopVar->SetArry(dynamic_cast<XlangRuntime*>(rt), pContext, vals);
                vals.clear();

                // If there's a filter condition, check it
                if (m_filterCond)
                {
                    Value filterValue;
                    ExecAction filterAction;
                    bOK = ExpExec(m_filterCond, rt, filterAction, pContext, filterValue);
                    if (!bOK)
                    {
                        delete pOutDict;
                        return false;
                    }
                    if (!filterValue.IsTrue())
                    {
                        continue;
                    }
                }

                // Evaluate key and value expressions
                Value keyValue, valValue;
                ExecAction keyAction, valAction;

                bOK = ExpExec(m_keyExpr, rt, keyAction, pContext, keyValue);
                if (!bOK)
                {
                    delete pOutDict;
                    return false;
                }

                bOK = ExpExec(m_valueExpr, rt, valAction, pContext, valValue);
                if (!bOK)
                {
                    delete pOutDict;
                    return false;
                }

                // Add to dict
                pOutDict->Set(keyValue, valValue);
            }

            v = X::Value(pOutDict);
            return true;
        }

        //=============================================================================
        // InlineIfOp::OpWithOperands
        // 
        // For expression: "yes" if x > 5 else "no"
        // When 'else' triggers precedence processing, InlineIfOp runs first.
        // 
        // At this point, operand stack has: [..., "yes", condition(x>5)]
        // (condition was pushed after 'if', before 'else')
        //
        // We create a PARTIAL TernaryOp with trueExpr and condition,
        // then InlineElseOp will complete it with falseExpr.
        //=============================================================================
        bool InlineIfOp::OpWithOperands(
            std::stack<AST::Expression*>& operands, int LeftTokenIndex)
        {
            // Need at least 2 operands: condition (top) and trueExpr
            if (operands.size() < 2)
            {
                return false;
            }

            // Pop condition (between 'if' and 'else')
            AST::Expression* condition = operands.top();
            operands.pop();

            // Pop true expression (left of 'if')
            AST::Expression* trueExpr = operands.top();
            operands.pop();

            // Create partial TernaryOp - falseExpr will be set by InlineElseOp
            TernaryOp* ternary = new TernaryOp();
            ternary->SetTrueExpr(trueExpr);
            ternary->SetCondition(condition);
            // falseExpr remains nullptr

            // Push partial ternary back
            operands.push(ternary);
            return true;
        }

        //=============================================================================
        // InlineElseOp::OpWithOperands
        // 
        // For expression: "yes" if x > 5 else "no"
        // After InlineIfOp ran, operand stack has: [..., TernaryOp(partial), "no"]
        // 
        // We pop "no" and the partial TernaryOp, complete it, push back.
        //=============================================================================
        bool InlineElseOp::OpWithOperands(
            std::stack<AST::Expression*>& operands, int LeftTokenIndex)
        {
            // Need at least 2 operands: falseExpr (top) and partial TernaryOp
            if (operands.size() < 2)
            {
                return false;
            }

            // Pop false expression (right of 'else')
            AST::Expression* falseExpr = operands.top();
            operands.pop();

            // Pop partial TernaryOp (from InlineIfOp)
            AST::Expression* top = operands.top();
            operands.pop();

            if (top->m_type != ObType::TernaryOp)
            {
                // Error: expected TernaryOp from InlineIfOp
                // Try to recover - push both back
                operands.push(top);
                operands.push(falseExpr);
                return false;
            }

            TernaryOp* ternary = dynamic_cast<TernaryOp*>(top);
            ternary->SetFalseExpr(falseExpr);

            // Push completed TernaryOp - this is now a single value
            operands.push(ternary);
            return true;
        }

    } // namespace AST
} // namespace X