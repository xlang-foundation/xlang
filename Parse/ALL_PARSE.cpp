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
// parse_all.cpp - Unity build file for Parse folder
// All Parse .cpp files included in one translation unit for maximum inlining
//=============================================================================

// Lexer and token
#include "lex.cpp"
#include "token.cpp"

// Parser
#include "parser.cpp"

// Operator registration and precedence
#include "action.cpp"
#include "precedence.cpp"

// Comprehension
#include "Comprehension.cpp"
