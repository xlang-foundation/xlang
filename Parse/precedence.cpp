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

/*
================================================================================
PRECEDENCE.CPP - Operator Precedence Configuration for XLang
================================================================================

This file contains all operator precedence settings, separated from action.cpp
for better maintainability. Precedences are based on Python's operator precedence.

Python Precedence (highest to lowest binding):
1.  () [] {} x.attr x() x[]     - Parentheses, subscription, call, attribute
2.  await                        - Await expression  
3.  **                           - Exponentiation (right-to-left)
4.  +x -x ~x                     - Unary plus, minus, bitwise NOT
5.  * / // % @                   - Multiplication, division, modulo
6.  + -                          - Addition, subtraction (binary)
7.  << >>                        - Bit shifts
8.  &                            - Bitwise AND
9.  ^                            - Bitwise XOR
10. |                            - Bitwise OR
11. == != < > <= >= is in not-in - Comparisons, membership, identity
12. not                          - Boolean NOT (unary)
13. and                          - Boolean AND
14. or                           - Boolean OR
15. if-else                      - Conditional expression (ternary)
16. lambda                       - Lambda expression
17. :=                           - Assignment expression (walrus)
--  =                            - Assignment (statement in Python, operator in XLang)

================================================================================
*/

#include "action.h"
#include "op_registry.h"

namespace X {

void RegisterPrecedence(OpRegistry* reg)
{
    //==========================================================================
    // HIGHEST PRECEDENCE - Binding, Subscription, Call, Attribute
    //==========================================================================
    
    // Parentheses, brackets, braces - highest binding
    RegOP("[", "]", "{", "}", "(", ")")
        .SetPrecedence(Precedence_High);
    
    // Attribute access - slightly higher than brackets for chaining
    RegOP(".", "..", "...")
        .SetPrecedence(Precedence_High1);

    //==========================================================================
    // AWAIT - Very high precedence (Python rank 2)
    //==========================================================================
    
    RegOP("await")
        .SetPrecedence(Precedence_High - 1);

    //==========================================================================
    // POWER OPERATOR - Higher than unary (Python rank 3)
    // Note: ** has right-to-left associativity
    //==========================================================================
    
    RegOP("**")
        .SetPrecedence(Precedence_Reqular + 5);

    //==========================================================================
    // UNARY OPERATORS (Python rank 4)
    // +x, -x, ~x - handled specially in parser (PreTokenIsOp check)
    // Not set here as they're context-dependent
    //==========================================================================

    //==========================================================================
    // MULTIPLICATIVE OPERATORS (Python rank 5)
    //==========================================================================
    
    RegOP("*", "/", "%", "//")
        .SetPrecedence(Precedence_Reqular + 4);

    //==========================================================================
    // ADDITIVE OPERATORS (Python rank 6)
    //==========================================================================
    
    RegOP("+", "-")
        .SetPrecedence(Precedence_Reqular + 3);

    //==========================================================================
    // SHIFT OPERATORS (Python rank 7)
    //==========================================================================
    
    RegOP("<<", ">>")
        .SetPrecedence(Precedence_Reqular + 2);

    //==========================================================================
    // BITWISE AND (Python rank 8)
    //==========================================================================
    
    RegOP("&")
        .SetPrecedence(Precedence_Reqular + 1);

    //==========================================================================
    // BITWISE XOR (Python rank 9)
    //==========================================================================
    
    RegOP("^")
        .SetPrecedence(Precedence_Reqular);

    //==========================================================================
    // BITWISE OR (Python rank 10)
    //==========================================================================
    
    RegOP("|")
        .SetPrecedence(Precedence_Reqular - 1);

    //==========================================================================
    // COMPARISON OPERATORS (Python rank 11)
    // All comparisons have same precedence: == != < > <= >= is in not-in
    //==========================================================================
    
    RegOP("==", "!=", "<", ">", "<=", ">=", "is")
        .SetPrecedence(Precedence_Reqular - 2);
    
    // 'in' is a comparison operator in Python, same precedence
    RegOP("in")
        .SetPrecedence(Precedence_Reqular - 2);

    //==========================================================================
    // BOOLEAN NOT (Python rank 12)
    // 'not' is unary, but needs precedence for cases like: not x in y
    //==========================================================================
    
    // Note: 'not' as unary is handled in SetProcess, but we can set precedence
    // for when it appears in expressions

    //==========================================================================
    // BOOLEAN AND (Python rank 13)
    //==========================================================================
    
    RegOP("and")
        .SetPrecedence(Precedence_Reqular - 3);

    //==========================================================================
    // BOOLEAN OR (Python rank 14)
    //==========================================================================
    
    RegOP("or")
        .SetPrecedence(Precedence_Reqular - 4);

    //==========================================================================
    // CONDITIONAL EXPRESSION - TERNARY if-else (Python rank 15)
    // For: value_if_true if condition else value_if_false
    // Must be LOWER than 'or' but HIGHER than '='
    //==========================================================================
    
    RegOP("if", "else")
        .SetPrecedence(Precedence_Reqular - 5);

    //==========================================================================
    // LAMBDA (Python rank 16)
    // Lambda expressions have very low precedence
    //==========================================================================
    
    // Lambda precedence can be handled if needed
    // RegOP("lambda").SetPrecedence(Precedence_Reqular - 6);

    //==========================================================================
    // ASSIGNMENT OPERATORS (Python: = is statement, XLang: lowest operator)
    // Must be LOWER than if-else so: x = a if c else b works correctly
    //==========================================================================
    
    RegOP("=", "+=", "-=", "*=", "/=", "%=", "//=")
        .SetPrecedence(Precedence_Reqular - 6);
    
    RegOP("**=", "&=", "|=", "^=", ">>=", "<<=")
        .SetPrecedence(Precedence_Reqular - 6);

    //==========================================================================
    // BLOCK STATEMENTS - Very low precedence
    // These consume the rest of the line as their condition/body
    //==========================================================================
    
    RegOP("elif", "while", "for")
        .SetPrecedence(Precedence_Reqular - 7);

    //==========================================================================
    // RETURN TYPE ANNOTATION
    //==========================================================================
    
    RegOP("->")
        .SetPrecedence(Precedence_Reqular);

    //==========================================================================
    // DECLARATION KEYWORDS
    //==========================================================================
    
    RegOP("const", "var", "namespace", "|-")
        .SetPrecedence(Precedence_Reqular + 4);

    //==========================================================================
    // SPECIAL OPERATORS
    //==========================================================================
    
    RegOP("as")
        .SetPrecedence(Precedence_LOW2 + 1);
    
    RegOP("deferred")
        .SetPrecedence(Precedence_LOW2 + 1);
    
    RegOP("thru")
        .SetPrecedence(Precedence_LOW2 - 1);
    
    RegOP("ref")
        .SetPrecedence(Precedence_Reqular);
    
    RegOP("extern", "nonlocal", "global")
        .SetPrecedence(Precedence_LOW2);

    //==========================================================================
    // IMPORT
    //==========================================================================
    
    RegOP("import")
        .SetPrecedence(Precedence_VERYLOW - 1);

    //==========================================================================
    // SEPARATORS
    //==========================================================================
    
    // Colon - for slicing, type annotations, dict literals
    // Needs to be lower than arithmetic so: t1[-20:120] works
    RegOP(":")
        .SetPrecedence(Precedence_VERYLOW + 1);
    
    // Newline and comma - very low to separate expressions
    RegOP("\n", ",")
        .SetPrecedence(Precedence_VERYLOW);

    //==========================================================================
    // SQL SUPPORT
    //==========================================================================
    
#if ADD_SQL
    RegOP("SELECT")
        .SetPrecedence(Precedence_LOW1 - 1);
#endif
}

} // namespace X


/*
================================================================================
PRECEDENCE SUMMARY TABLE (Highest to Lowest)
================================================================================

| Level              | Value (relative)  | Operators                          |
|--------------------|-------------------|------------------------------------|
| Precedence_High1   | (highest)         | . .. ...                           |
| Precedence_High    |                   | () [] {}                           |
| High - 1           |                   | await                              |
| Reqular + 5        |                   | **                                 |
| Reqular + 4        |                   | * / % //  (also: const var etc.)   |
| Reqular + 3        |                   | + - (binary)                       |
| Reqular + 2        |                   | << >>                              |
| Reqular + 1        |                   | &                                  |
| Reqular            |                   | ^  (also: -> ref)                  |
| Reqular - 1        |                   | |                                  |
| Reqular - 2        |                   | == != < > <= >= is in              |
| Reqular - 3        |                   | and                                |
| Reqular - 4        |                   | or                                 |
| Reqular - 5        |                   | if else (ternary)                  |
| Reqular - 6        |                   | = += -= *= /= etc.                 |
| Reqular - 7        |                   | elif while for (block statements)  |
| LOW2 + 1           |                   | as deferred                        |
| LOW2               |                   | extern nonlocal global             |
| LOW2 - 1           |                   | thru                               |
| LOW1 - 1           |                   | SELECT                             |
| VERYLOW + 1        |                   | :                                  |
| VERYLOW            |                   | \n ,                               |
| VERYLOW - 1        | (lowest)          | import                             |

================================================================================
USAGE IN action.cpp
================================================================================

In action.cpp, replace the precedence section (lines ~530-595) with:

    // Call precedence registration from precedence.cpp
    RegisterPrecedence(reg);

And add the declaration in action.h:

    void RegisterPrecedence(OpRegistry* reg);

================================================================================
*/
