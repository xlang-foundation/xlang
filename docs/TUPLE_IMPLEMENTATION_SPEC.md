# XLang Tuple Implementation - Test Specification

## Overview
This document specifies the tuple functionality to be added to XLang, based on Python's tuple behavior.

## Test File
`test2026/general/tuple_ops.x` - Contains 18 comprehensive test checkpoints

## Core Features to Implement

### 1. Tuple Creation
- **Explicit creation with parentheses**: `t = (1, 2, 3)`
- **Implicit creation (comma operator)**: `t = 1, 2, 3`
- **Single-element tuple**: `t = (42,)` (requires trailing comma)
- **Empty tuple**: `t = ()`

### 2. Tuple Indexing
- **Positive indexing**: `t[0]`, `t[1]`, `t[2]`
- **Negative indexing**: `t[-1]`, `t[-2]` (last, second-to-last)
- **Index bounds checking**: Raise error for out-of-bounds access

### 3. Tuple Unpacking
- **Basic unpacking**: `a, b, c = (1, 2, 3)`
- **Direct unpacking**: `x, y = 10, 20`
- **Swap values**: `a, b = b, a`
- **Unpacking from function returns**: `x, y = get_point()`

### 4. Multiple Return Values
- **Function returns tuple**: `def func(): return 1, 2, 3`
- **Auto-tuple creation for multiple returns**: Comma-separated return values become tuple
- **Unpacking on receive**: `a, b = func()`
- **Single variable receives whole tuple**: `result = func()`

### 5. Tuple Operations
- **Length**: `len(t)` - returns number of elements
- **Indexing**: `t[i]` - access element at index i
- **Slicing**: `t[start:end]` - create sub-tuple
- **Concatenation**: `t1 + t2` - combine tuples
- **Repetition**: `t * n` - repeat tuple n times
- **Contains**: `val in t` - check membership
- **Iteration**: `for item in t` - iterate over elements

### 6. Immutability (Important!)
- **Tuples should be immutable**: Once created, elements cannot be changed
- **Assignment should fail**: `t[0] = 5` should raise an error
- **Modification methods not supported**: No append, remove, etc.

### 7. Nested Tuples
- **Tuples can contain tuples**: `t = (1, (2, 3), 4)`
- **Access nested elements**: `t[1][0]` returns 2
- **Unpacking nested**: `a, (b, c), d = (1, (2, 3), 4)`

### 8. Mixed Types
- **Tuples can contain any types**: `t = (1, "hello", 3.14, True, [1,2,3])`
- **Type consistency not required**: Unlike some static languages

### 9. Empty and Single-Element Tuples
- **Empty tuple**: `t = ()`
- **Single element requires comma**: `t = (5,)` (not `t = (5)` which is just parentheses)

## Implementation Notes

### ObjType Addition
Add to `Api/xlang.h` enum class ObjType:
```cpp
Tuple,  // Add after List or before Dict
```

### Core Files to Create/Modify
1. **Core/tuple.h** - Tuple class definition
2. **Core/tuple.cpp** - Tuple implementation
3. **Main/ALL.cpp** - Include tuple initialization

### Key Design Decisions

#### Storage
- Use `std::vector<X::Value>` internally (like List)
- Mark as immutable after construction

#### Unpacking
- Parser needs to recognize tuple unpacking syntax
- Assignment operator handles multiple left-hand values
- Use special AST node for tuple unpacking

#### Function Returns
- When function has multiple return statements with commas, auto-create tuple
- Example: `return a, b, c` becomes `return Tuple(a, b, c)`

#### Syntax Support
- **Creation**: `(val1, val2, ...)` or `val1, val2, ...`
- **Unpacking**: `var1, var2 = tuple_expr`
- **In conditions**: Evaluate to truthy/falsy based on length (empty = false, non-empty = true)

## Test Coverage

| Test | Feature | Checkpoint |
|------|---------|-----------|
| cp1 | Explicit tuple creation | `t = (1, 2, 3)` |
| cp2 | Implicit tuple creation | `t = 1, 2, 3` |
| cp3 | Single element tuple | `t = (42,)` |
| cp4 | Positive indexing | `t[0]` |
| cp5 | Negative indexing | `t[-1]` |
| cp6 | Basic unpacking | `a, b, c = t` |
| cp7 | Direct unpacking | `x, y, z = 1, 2, 3` |
| cp8 | Value swapping | `a, b = b, a` |
| cp9 | Function return tuple | `return x, y` |
| cp10 | Unpack function return | `x, y = func()` |
| cp11 | Mixed types | `(1, "str", 3.14, True)` |
| cp12 | Nested tuples | `(1, (2, 3), 4)` |
| cp13 | Nested access | `t[1][0]` |
| cp14 | Length | `len(t)` |
| cp15 | Slicing | `t[1:4]` |
| cp16 | Concatenation | `t1 + t2` |
| cp17 | Repetition | `t * 3` |
| cp18 | Iteration | `for x in t` |

## Expected Behavior Examples

### Creation
```python
# All create tuples
t1 = (1, 2, 3)
t2 = 1, 2, 3
t3 = tuple([1, 2, 3])  # from list

# Single element
t4 = (5,)    # tuple with one element
t5 = (5)     # NOT a tuple, just int in parentheses
```

### Unpacking
```python
# Basic
a, b, c = (1, 2, 3)  # a=1, b=2, c=3

# Swap
x, y = y, x

# Function
def get_user():
    return "John", 25, "NYC"

name, age, city = get_user()
```

### Operations
```python
t = (1, 2, 3)
t[0]        # 1
t[-1]       # 3
t[1:3]      # (2, 3)
len(t)      # 3
t + (4,5)   # (1, 2, 3, 4, 5)
t * 2       # (1, 2, 3, 1, 2, 3)
2 in t      # True
```

## Priority Implementation Order

1. **Phase 1: Basic Creation & Access**
   - Tuple object class
   - Parenthesized creation: `(a, b, c)`
   - Indexing: `t[i]`
   - Length: `len(t)`

2. **Phase 2: Multi-Return Values**
   - Function return multiple values
   - Auto-tuple for comma returns
   - Single variable capture

3. **Phase 3: Unpacking**
   - AST support for multiple assignment targets
   - Basic unpacking: `a, b = tuple`
   - Direct unpacking: `x, y = 1, 2`

4. **Phase 4: Operations**
   - Slicing
   - Concatenation
   - Repetition
   - Iteration

5. **Phase 5: Advanced Features**
   - Nested tuples
   - Immutability enforcement
   - Edge cases (empty, single-element)

## Notes for Implementation

- **Parser Changes**: Need to handle comma as tuple creator vs separator
- **AST Nodes**: May need TupleExpr and UnpackExpr nodes
- **Precedence**: Comma operator has lower precedence than most operators
- **Parentheses**: Sometimes optional, sometimes required (function calls, etc.)

Please review this test specification and let me know if you'd like any changes before implementation!
