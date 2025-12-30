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
#include "range.h"

namespace X
{
    namespace AST
    {
        //=============================================================================
        // Fast path for Range - no virtual calls, no vector, no lock
        //=============================================================================
        FORCE_INLINE bool ExecRangeFastPath(XlangRuntime* rt, ExecAction& action,
            XObj* pContext, Value& v, Data::Range* pRange,
            Expression* filterCond,Expression* loopVar,
            Expression* outputExpr)
        {
            // Get range parameters once - no virtual calls in loop
            const long long start = pRange->GetStart();
            const long long stop = pRange->GetStop();
            const long long step = pRange->GetStep();

            // Pre-allocate output list
            long long estimatedSize = (stop - start) / step;
            if (filterCond) estimatedSize /= 2;  // Rough estimate with filter

            X::Data::List* pOutList = new X::Data::List();
 
            // Get loop variable's LValue once - avoid repeated lookups
            Value varValue;
            LValue varLValue = nullptr;
            loopVar->Exec(rt, action, pContext, varValue, &varLValue);

            //auto loopStart = std::chrono::high_resolution_clock::now();

            // Native for loop - no virtual calls, no vector, no lock!
            for (long long i = start; i < stop; i += step)
            {
                // Direct assignment to loop variable
                if (varLValue)
                {
                    *varLValue = Value(i);
                }
                else
                {
                    // Fallback: use SetArry
                    std::vector<Value> vals = { Value(i), Value(i) };
                    loopVar->SetArry(rt, pContext, vals);
                }

                // Filter condition
                if (filterCond)
                {
                    Value filterValue;
                    ExecAction filterAction;
                    if (!ExpExec(filterCond, rt, filterAction, pContext, filterValue))
                    {
                        delete pOutList;
                        return false;
                    }
                    if (!filterValue.IsTrue())
                    {
                        continue;
                    }
                }

                // Output expression
                Value outValue;
                ExecAction outAction;
                if (ExpExec(outputExpr, rt, outAction, pContext, outValue))
                {
                    pOutList->FastAdd(outValue);
                }
            }

            //auto loopEnd = std::chrono::high_resolution_clock::now();
            //double loopTime = std::chrono::duration<double, std::milli>(loopEnd - loopStart).count();
            //std::cout << "Range fast path time: " << loopTime << " ms" << std::endl;

            v = X::Value(pOutList);
            return true;
        }

        //=============================================================================
        // Generic path for other iterables (list, dict, etc.)
        //=============================================================================
        FORCE_INLINE bool ExecGenericPath(XlangRuntime* rt, ExecAction& action,
            XObj* pContext, Value& v, Data::Object* pDataObj,
            Expression* filterCond, Expression* loopVar,
            Expression* outputExpr)
        {
            X::Data::List* pOutList = new X::Data::List();
            X::Data::Iterator_Pos curPos = nullptr;
            std::vector<Value> vals;

            //auto loopStart = std::chrono::high_resolution_clock::now();

            while (pDataObj->GetAndUpdatePos(curPos, vals, false))
            {
                loopVar->SetArry(rt, pContext, vals);
                vals.clear();

                if (filterCond)
                {
                    Value filterValue;
                    ExecAction filterAction;
                    if (!ExpExec(filterCond, rt, filterAction, pContext, filterValue))
                    {
                        delete pOutList;
                        return false;
                    }
                    if (!filterValue.IsTrue())
                    {
                        continue;
                    }
                }

                Value outValue;
                ExecAction outAction;
                if (ExpExec(outputExpr, rt, outAction, pContext, outValue))
                {
                    pOutList->Add(rt, outValue);
                }
            }

            //auto loopEnd = std::chrono::high_resolution_clock::now();
            //double loopTime = std::chrono::duration<double, std::milli>(loopEnd - loopStart).count();
            //std::cout << "Generic path time: " << loopTime << " ms" << std::endl;

            v = X::Value(pOutList);
            return true;
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

            // Check if it's a Range - use fast path
            if (pDataObj->GetType() == X::ObjType::Range)
            {
                return ExecRangeFastPath(rt, action, pContext, v,
                    dynamic_cast<Data::Range*>(pDataObj), m_filterCond, m_loopVar,
                    m_outputExpr);
            }

            // Generic path for other iterables
            return ExecGenericPath(rt, action, pContext, v, pDataObj, m_filterCond,
                m_loopVar, m_outputExpr);
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
            int maxTokenIndex = -1;
            if (trueExpr)
            {
                ternary->ReCalcHint(trueExpr);
                if (maxTokenIndex < trueExpr->GetTokenIndex())
                {
                    maxTokenIndex = trueExpr->GetTokenIndex();
                }
            }
            if (condition)
            {
                ternary->ReCalcHint(condition);
                if (maxTokenIndex < condition->GetTokenIndex())
                {
                    maxTokenIndex = condition->GetTokenIndex();
                }
            }
            ternary->SetTokenIndex(maxTokenIndex);
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
            if (falseExpr)
            {
                ternary->ReCalcHint(falseExpr);
                if (ternary->GetTokenIndex() < falseExpr->GetTokenIndex())
                {
                    ternary->SetTokenIndex(falseExpr->GetTokenIndex());
                }
            }

            // Push completed TernaryOp - this is now a single value
            operands.push(ternary);
            return true;
        }

        bool InlineForOp::OpWithOperands(
            std::stack<AST::Expression*>& operands, 
            int LeftTokenIndex)
        {
            return false;
        }

} // namespace AST
} // namespace X