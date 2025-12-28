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
#include "value.h"
#include "def.h"
#include <cmath>

namespace X {
    namespace AST {

        //=============================================================================
        // FastBinaryOp - Inlined switch for ALL binary operators
        // Returns true if handled, false if fallback needed
        //
        // REQUIREMENT: Add new OP_IDs to your enum in def.h:
        //   Add, Sub, Mul, Div, Mod, FloorDiv, Power,
        //   BitAnd, BitOr, BitXor, LeftShift, RightShift
        //
        // Then register them in action.cpp:
        //   RegOP("+").SetId(reg, OP_ID::Add);
        //   RegOP("-").SetId(reg, OP_ID::Sub);
        //   etc.
        //=============================================================================
        FORCE_INLINE bool FastBinaryOp(OP_ID opId, X::Value& L, X::Value& R, X::Value& v)
        {
            switch (opId)
            {
                //=========================================================================
                // Arithmetic Operators
                //=========================================================================
            case OP_ID::Add:
                v = L + R;
                return true;
            case OP_ID::AddEqu:
                v = L + R;
                return true;
            case OP_ID::Sub:
                v = L - R;
                return true;

            case OP_ID::Mul:
                v = L * R;
                return true;

            case OP_ID::Div:
                v = L / R;
                return true;

            case OP_ID::Mod:
                v = (long long)L % (long long)R;
                return true;

            case OP_ID::FloorDiv:
                v = L.ToLongLong() / R.ToLongLong();
                return true;

            case OP_ID::Power:
                v = std::pow(L.ToDouble(), R.ToDouble());
                return true;

                //=========================================================================
                // Comparison Operators
                //=========================================================================
            case OP_ID::Equal:
            case OP_ID::IsEqual:
                v = X::Value(L == R);
                return true;

            case OP_ID::NotEqual:
                v = X::Value(L != R);
                return true;

            case OP_ID::Great:
                v = X::Value(L > R);
                return true;

            case OP_ID::Less:
                v = X::Value(L < R);
                return true;

            case OP_ID::GreatAndEqual:
                v = X::Value(L >= R);
                return true;

            case OP_ID::LessAndEqual:
                v = X::Value(L <= R);
                return true;

                //=========================================================================
                // Logical Operators
                //=========================================================================
            case OP_ID::And:
                v = X::Value(L.IsTrue() && R.IsTrue());
                return true;

            case OP_ID::Or:
                v = X::Value(L.IsTrue() || R.IsTrue());
                return true;

                //=========================================================================
                // Bitwise Operators
                //=========================================================================
            case OP_ID::BitAnd:
                v = L.ToLongLong() & R.ToLongLong();
                return true;

            case OP_ID::BitOr:
                v = L.ToLongLong() | R.ToLongLong();
                return true;

            case OP_ID::BitXor:
                v = L.ToLongLong() ^ R.ToLongLong();
                return true;

            case OP_ID::LeftShift:
                v = L.ToLongLong() << R.ToLongLong();
                return true;

            case OP_ID::RightShift:
                v = L.ToLongLong() >> R.ToLongLong();
                return true;

            default:
                return false;  // Not handled - use fallback
            }
        }

        //=============================================================================
        // FastUnaryOp - Inlined switch for ALL unary operators
        //=============================================================================
        FORCE_INLINE bool FastUnaryOp(OP_ID opId, X::Value& R, X::Value& v)
        {
            switch (opId)
            {
            case OP_ID::UnaryPlus:
                v = R;
                return true;

            case OP_ID::UnaryMinus:
                if (R.GetType() == ValueType::Double)
                {
                    v = X::Value(0.0);
                }
                else
                {
                    v = X::Value(0);
                }
                v -= R;
                return true;

            case OP_ID::NotOp:
                v = X::Value(R.IsZero());
                return true;

            case OP_ID::BitNot:
                v = ~R.ToLongLong();
                return true;

            case OP_ID::LogicalNot:
                v = R.IsTrue() ? X::Value(false) : X::Value(true);
                return true;

            default:
                return false;
            }
        }

        //=============================================================================
        // FastBinaryOpWithFallback - Try fast path, fallback to function pointer
        //=============================================================================
        FORCE_INLINE bool FastBinaryOpWithFallback(
            OP_ID opId,
            short Op,
            XlangRuntime* rt,
            BinaryOp* op,
            X::Value& L,
            X::Value& R,
            X::Value& v)
        {
            // Try fast path first
            if (FastBinaryOp(opId, L, R, v))
            {
                return true;
            }

            // Fallback to function pointer for complex/custom operators
            auto func = G::I().R().OpAct(Op).binaryop;
            return func ? func(rt, op, L, R, v) : false;
        }

        //=============================================================================
        // FastUnaryOpWithFallback - Try fast path, fallback to function pointer
        //=============================================================================
        FORCE_INLINE bool FastUnaryOpWithFallback(
            OP_ID opId,
            short Op,
            XlangRuntime* rt,
            UnaryOp* op,
            X::Value& R,
            X::Value& v)
        {
            // Try fast path first
            if (FastUnaryOp(opId, R, v))
            {
                return true;
            }

            // Fallback to function pointer
            auto func = G::I().R().OpAct(Op).unaryop;
            return func ? func(rt, op, R, v) : false;
        }

    } // namespace AST
} // namespace X