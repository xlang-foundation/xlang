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
#include "list.h"

namespace X
{
namespace AST
{

//=============================================================================
// TernaryOp - Inline conditional expression
// Syntax: value_if_true if condition else value_if_false
// Example: x = 10 if a > 5 else 20
//=============================================================================
class TernaryOp :
    public Expression
{
protected:
    Expression* m_condition = nullptr;    // The condition to evaluate
    Expression* m_trueExpr = nullptr;     // Expression if condition is true
    Expression* m_falseExpr = nullptr;    // Expression if condition is false

public:
    TernaryOp() : Expression()
    {
        m_type = ObType::TernaryOp;
    }

    TernaryOp(Expression* trueExpr, Expression* condition, Expression* falseExpr)
        : Expression()
    {
        m_type = ObType::TernaryOp;
        SetTrueExpr(trueExpr);
        SetCondition(condition);
        SetFalseExpr(falseExpr);
    }

    ~TernaryOp()
    {
        if (m_condition) delete m_condition;
        if (m_trueExpr) delete m_trueExpr;
        if (m_falseExpr) delete m_falseExpr;
    }

    // Setters with parent linkage
    void SetCondition(Expression* cond)
    {
        m_condition = cond;
        if (m_condition)
        {
            ReCalcHint(m_condition);
            m_condition->SetParent(this);
        }
    }

    void SetTrueExpr(Expression* expr)
    {
        m_trueExpr = expr;
        if (m_trueExpr)
        {
            ReCalcHint(m_trueExpr);
            m_trueExpr->SetParent(this);
        }
    }

    void SetFalseExpr(Expression* expr)
    {
        m_falseExpr = expr;
        if (m_falseExpr)
        {
            ReCalcHint(m_falseExpr);
            m_falseExpr->SetParent(this);
        }
    }

    // Getters
    FORCE_INLINE Expression* GetCondition() { return m_condition; }
    FORCE_INLINE Expression* GetTrueExpr() { return m_trueExpr; }
    FORCE_INLINE Expression* GetFalseExpr() { return m_falseExpr; }

    virtual void SetScope(Scope* p) override
    {
        Expression::SetScope(p);
        if (m_condition) m_condition->SetScope(p);
        if (m_trueExpr) m_trueExpr->SetScope(p);
        if (m_falseExpr) m_falseExpr->SetScope(p);
    }

    virtual void ScopeLayout() override
    {
        if (m_condition) m_condition->ScopeLayout();
        if (m_trueExpr) m_trueExpr->ScopeLayout();
        if (m_falseExpr) m_falseExpr->ScopeLayout();
    }

    virtual bool CalcCallables(XlangRuntime* rt, XObj* pContext,
        std::vector<AST::Expression*>& callables) override
    {
        bool bHave = false;
        if (m_condition) bHave |= m_condition->CalcCallables(rt, pContext, callables);
        if (m_trueExpr) bHave |= m_trueExpr->CalcCallables(rt, pContext, callables);
        if (m_falseExpr) bHave |= m_falseExpr->CalcCallables(rt, pContext, callables);
        return bHave;
    }

    virtual int GetLeftMostCharPos() override
    {
        int pos = GetCharPos();
        int startLine = GetStartLine();
        if (m_trueExpr)
        {
            int posT = m_trueExpr->GetLeftMostCharPos();
            if (posT < pos && m_trueExpr->GetStartLine() <= startLine)
            {
                pos = posT;
            }
        }
        return pos;
    }

    virtual bool ToBytes(XlangRuntime* rt, XObj* pContext, X::XLangStream& stream) override
    {
        Expression::ToBytes(rt, pContext, stream);
        SaveToStream(rt, pContext, m_condition, stream);
        SaveToStream(rt, pContext, m_trueExpr, stream);
        SaveToStream(rt, pContext, m_falseExpr, stream);
        return true;
    }

    virtual bool FromBytes(X::XLangStream& stream) override
    {
        Expression::FromBytes(stream);
        m_condition = BuildFromStream<Expression>(stream);
        m_trueExpr = BuildFromStream<Expression>(stream);
        m_falseExpr = BuildFromStream<Expression>(stream);
        return true;
    }

    virtual bool Exec(XlangRuntime* rt, ExecAction& action, XObj* pContext,
        Value& v, LValue* lValue = nullptr) override;
};

//=============================================================================
// ListComprehension - Inline list generation
// Syntax: [expr for var in iterable]
//         [expr for var in iterable if condition]
// Example: squares = [x*x for x in range(10)]
//          evens = [x for x in range(20) if x % 2 == 0]
//=============================================================================
class ListComprehension :
    public Expression
{
protected:
    Expression* m_outputExpr = nullptr;   // The expression to evaluate for each item
    Expression* m_loopVar = nullptr;      // Loop variable(s)
    Expression* m_iterable = nullptr;     // The iterable to loop over
    Expression* m_filterCond = nullptr;   // Optional filter condition (if clause)

public:
    ListComprehension() : Expression()
    {
        m_type = ObType::ListComprehension;
    }

    ListComprehension(Expression* outputExpr, Expression* loopVar,
        Expression* iterable, Expression* filterCond = nullptr)
        : Expression()
    {
        m_type = ObType::ListComprehension;
        SetOutputExpr(outputExpr);
        SetLoopVar(loopVar);
        SetIterable(iterable);
        SetFilterCond(filterCond);
    }

    ~ListComprehension()
    {
        if (m_outputExpr) delete m_outputExpr;
        if (m_loopVar) delete m_loopVar;
        if (m_iterable) delete m_iterable;
        if (m_filterCond) delete m_filterCond;
    }

    // Setters with parent linkage
    void SetOutputExpr(Expression* expr)
    {
        m_outputExpr = expr;
        if (m_outputExpr)
        {
            ReCalcHint(m_outputExpr);
            m_outputExpr->SetParent(this);
        }
    }

    void SetLoopVar(Expression* var)
    {
        m_loopVar = var;
        if (m_loopVar)
        {
            ReCalcHint(m_loopVar);
            m_loopVar->SetParent(this);
            m_loopVar->SetIsLeftValue(true);  // Loop var is assigned to
        }
    }

    void SetIterable(Expression* iter)
    {
        m_iterable = iter;
        if (m_iterable)
        {
            ReCalcHint(m_iterable);
            m_iterable->SetParent(this);
        }
    }

    void SetFilterCond(Expression* cond)
    {
        m_filterCond = cond;
        if (m_filterCond)
        {
            ReCalcHint(m_filterCond);
            m_filterCond->SetParent(this);
        }
    }

    // Getters
    FORCE_INLINE Expression* GetOutputExpr() { return m_outputExpr; }
    FORCE_INLINE Expression* GetLoopVar() { return m_loopVar; }
    FORCE_INLINE Expression* GetIterable() { return m_iterable; }
    FORCE_INLINE Expression* GetFilterCond() { return m_filterCond; }
    FORCE_INLINE bool HasFilter() { return m_filterCond != nullptr; }

    virtual void SetScope(Scope* p) override
    {
        Expression::SetScope(p);
        if (m_outputExpr) m_outputExpr->SetScope(p);
        if (m_loopVar) m_loopVar->SetScope(p);
        if (m_iterable) m_iterable->SetScope(p);
        if (m_filterCond) m_filterCond->SetScope(p);
    }

    virtual void ScopeLayout() override
    {
        // Loop variable needs scope layout first as it defines a variable
        if (m_loopVar) m_loopVar->ScopeLayout();
        if (m_iterable) m_iterable->ScopeLayout();
        if (m_outputExpr) m_outputExpr->ScopeLayout();
        if (m_filterCond) m_filterCond->ScopeLayout();
    }

    virtual bool CalcCallables(XlangRuntime* rt, XObj* pContext,
        std::vector<AST::Expression*>& callables) override
    {
        bool bHave = false;
        if (m_outputExpr) bHave |= m_outputExpr->CalcCallables(rt, pContext, callables);
        if (m_loopVar) bHave |= m_loopVar->CalcCallables(rt, pContext, callables);
        if (m_iterable) bHave |= m_iterable->CalcCallables(rt, pContext, callables);
        if (m_filterCond) bHave |= m_filterCond->CalcCallables(rt, pContext, callables);
        return bHave;
    }

    virtual int GetLeftMostCharPos() override
    {
        // List comprehension typically starts with '['
        return GetCharPos();
    }

    virtual bool ToBytes(XlangRuntime* rt, XObj* pContext, X::XLangStream& stream) override
    {
        Expression::ToBytes(rt, pContext, stream);
        SaveToStream(rt, pContext, m_outputExpr, stream);
        SaveToStream(rt, pContext, m_loopVar, stream);
        SaveToStream(rt, pContext, m_iterable, stream);
        SaveToStream(rt, pContext, m_filterCond, stream);
        return true;
    }

    virtual bool FromBytes(X::XLangStream& stream) override
    {
        Expression::FromBytes(stream);
        m_outputExpr = BuildFromStream<Expression>(stream);
        m_loopVar = BuildFromStream<Expression>(stream);
        m_iterable = BuildFromStream<Expression>(stream);
        m_filterCond = BuildFromStream<Expression>(stream);
        return true;
    }

    virtual bool Exec(XlangRuntime* rt, ExecAction& action, XObj* pContext,
        Value& v, LValue* lValue = nullptr) override;
};

//=============================================================================
// DictComprehension - Inline dictionary generation
// Syntax: {key_expr: value_expr for var in iterable}
//         {key_expr: value_expr for var in iterable if condition}
// Example: squares = {x: x*x for x in range(10)}
//=============================================================================
class DictComprehension :
    public Expression
{
protected:
    Expression* m_keyExpr = nullptr;      // Key expression
    Expression* m_valueExpr = nullptr;    // Value expression
    Expression* m_loopVar = nullptr;      // Loop variable(s)
    Expression* m_iterable = nullptr;     // The iterable to loop over
    Expression* m_filterCond = nullptr;   // Optional filter condition

public:
    DictComprehension() : Expression()
    {
        m_type = ObType::DictComprehension;
    }

    DictComprehension(Expression* keyExpr, Expression* valueExpr,
        Expression* loopVar, Expression* iterable, Expression* filterCond = nullptr)
        : Expression()
    {
        m_type = ObType::DictComprehension;
        SetKeyExpr(keyExpr);
        SetValueExpr(valueExpr);
        SetLoopVar(loopVar);
        SetIterable(iterable);
        SetFilterCond(filterCond);
    }

    ~DictComprehension()
    {
        if (m_keyExpr) delete m_keyExpr;
        if (m_valueExpr) delete m_valueExpr;
        if (m_loopVar) delete m_loopVar;
        if (m_iterable) delete m_iterable;
        if (m_filterCond) delete m_filterCond;
    }

    // Setters
    void SetKeyExpr(Expression* expr)
    {
        m_keyExpr = expr;
        if (m_keyExpr)
        {
            ReCalcHint(m_keyExpr);
            m_keyExpr->SetParent(this);
        }
    }

    void SetValueExpr(Expression* expr)
    {
        m_valueExpr = expr;
        if (m_valueExpr)
        {
            ReCalcHint(m_valueExpr);
            m_valueExpr->SetParent(this);
        }
    }

    void SetLoopVar(Expression* var)
    {
        m_loopVar = var;
        if (m_loopVar)
        {
            ReCalcHint(m_loopVar);
            m_loopVar->SetParent(this);
            m_loopVar->SetIsLeftValue(true);
        }
    }

    void SetIterable(Expression* iter)
    {
        m_iterable = iter;
        if (m_iterable)
        {
            ReCalcHint(m_iterable);
            m_iterable->SetParent(this);
        }
    }

    void SetFilterCond(Expression* cond)
    {
        m_filterCond = cond;
        if (m_filterCond)
        {
            ReCalcHint(m_filterCond);
            m_filterCond->SetParent(this);
        }
    }

    // Getters
    FORCE_INLINE Expression* GetKeyExpr() { return m_keyExpr; }
    FORCE_INLINE Expression* GetValueExpr() { return m_valueExpr; }
    FORCE_INLINE Expression* GetLoopVar() { return m_loopVar; }
    FORCE_INLINE Expression* GetIterable() { return m_iterable; }
    FORCE_INLINE Expression* GetFilterCond() { return m_filterCond; }
    FORCE_INLINE bool HasFilter() { return m_filterCond != nullptr; }

    virtual void SetScope(Scope* p) override
    {
        Expression::SetScope(p);
        if (m_keyExpr) m_keyExpr->SetScope(p);
        if (m_valueExpr) m_valueExpr->SetScope(p);
        if (m_loopVar) m_loopVar->SetScope(p);
        if (m_iterable) m_iterable->SetScope(p);
        if (m_filterCond) m_filterCond->SetScope(p);
    }

    virtual void ScopeLayout() override
    {
        if (m_loopVar) m_loopVar->ScopeLayout();
        if (m_iterable) m_iterable->ScopeLayout();
        if (m_keyExpr) m_keyExpr->ScopeLayout();
        if (m_valueExpr) m_valueExpr->ScopeLayout();
        if (m_filterCond) m_filterCond->ScopeLayout();
    }

    virtual bool CalcCallables(XlangRuntime* rt, XObj* pContext,
        std::vector<AST::Expression*>& callables) override
    {
        bool bHave = false;
        if (m_keyExpr) bHave |= m_keyExpr->CalcCallables(rt, pContext, callables);
        if (m_valueExpr) bHave |= m_valueExpr->CalcCallables(rt, pContext, callables);
        if (m_loopVar) bHave |= m_loopVar->CalcCallables(rt, pContext, callables);
        if (m_iterable) bHave |= m_iterable->CalcCallables(rt, pContext, callables);
        if (m_filterCond) bHave |= m_filterCond->CalcCallables(rt, pContext, callables);
        return bHave;
    }

    virtual bool ToBytes(XlangRuntime* rt, XObj* pContext, X::XLangStream& stream) override
    {
        Expression::ToBytes(rt, pContext, stream);
        SaveToStream(rt, pContext, m_keyExpr, stream);
        SaveToStream(rt, pContext, m_valueExpr, stream);
        SaveToStream(rt, pContext, m_loopVar, stream);
        SaveToStream(rt, pContext, m_iterable, stream);
        SaveToStream(rt, pContext, m_filterCond, stream);
        return true;
    }

    virtual bool FromBytes(X::XLangStream& stream) override
    {
        Expression::FromBytes(stream);
        m_keyExpr = BuildFromStream<Expression>(stream);
        m_valueExpr = BuildFromStream<Expression>(stream);
        m_loopVar = BuildFromStream<Expression>(stream);
        m_iterable = BuildFromStream<Expression>(stream);
        m_filterCond = BuildFromStream<Expression>(stream);
        return true;
    }

    virtual bool Exec(XlangRuntime* rt, ExecAction& action, XObj* pContext,
        Value& v, LValue* lValue = nullptr) override;
};

//=============================================================================
// InlineElseOp - Operator to handle 'else' in ternary expressions
// This is used during parsing to build TernaryOp
//=============================================================================
class InlineElseOp :
    public Operator
{
public:
    InlineElseOp() : Operator()
    {
        m_type = ObType::InlineElseOp;
    }

    InlineElseOp(short op) : Operator(op)
    {
        m_type = ObType::InlineElseOp;
    }

    virtual bool OpWithOperands(
        std::stack<AST::Expression*>& operands, int LeftTokenIndex) override;
};

//=============================================================================
// InlineIfOp - Operator to handle 'if' in ternary expressions
// This is used during parsing to build TernaryOp
//=============================================================================
class InlineIfOp :
    public Operator
{
public:
    InlineIfOp() : Operator()
    {
        m_type = ObType::InlineIfOp;
    }

    InlineIfOp(short op) : Operator(op)
    {
        m_type = ObType::InlineIfOp;
    }

    virtual bool OpWithOperands(
        std::stack<AST::Expression*>& operands, int LeftTokenIndex) override;
};

} // namespace AST
} // namespace X
