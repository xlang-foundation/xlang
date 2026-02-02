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

//=============================================================================
// ast_all.cpp - Unity build file for AST folder
// All AST .cpp files included in one translation unit for maximum inlining
//=============================================================================

// Core expression and operator
#include "exp.cpp"
#include "op.cpp"
#include "var.cpp"
#include "number.cpp"
#include "pair.cpp"

// Operators
#include "dotop.cpp"
#include "feedop.cpp"
#include "await.cpp"
#include "inline_expr.cpp"

// Block and scope
#include "block.cpp"
#include "blockstate.cpp"
#include "metascope.cpp"

// Function and class
#include "func.cpp"
#include "xclass.cpp"
#include "decor.cpp"

// Module and namespace
#include "module.cpp"
#include "namespace_var.cpp"

// JIT and inline
#include "jitblock.cpp"
#include "InlineCall.cpp"

