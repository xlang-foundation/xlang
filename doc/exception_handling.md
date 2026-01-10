# Exception Handling Implementation in XLang

## Overview
XLang implements a Python-like `try-except-finally-raise` exception handling mechanism.
Exceptions are propagated through the call stack using `ExecActionType::Throw` and a `Value` payload.

## Components

### AST Nodes
- `Try`: Manages execution of the try block, matching against `except` clauses, and executing `finally` block.
- `Except`: Represents an exception handler. Supports optional variable binding (`except e`).
- `Finally`: Represents code that must execute regardless of outcome.
- `Raise`: A unary operator (`UnaryOp`) that initiates an exception.

### Execution Flow
1. **Raise**:
   - `UnaryOp::Exec_D` handles `OP_ID::Raise`.
   - Sets `action.type = ExecActionType::Throw`.
   - Sets `action.exceptionValue = operand`.
   - Returns `true`.

2. **Block Execution (`Block::Exec_i`)**:
   - Loop checks `action.type`.
   - If `Throw`, breaks loop and propagates action up.
   - Also checks `rt->GetException()` to handle exceptions from function calls.

3. **Function Boundary (`Func::Call`)**:
   - Catches `Throw` action from function body.
   - Sets `rt->SetException(value)` (in `XlangRuntime`).
   - Returns `true` to allow caller to proceed and check `rt` for exceptions.

4. **Try Block (`Try::Exec`)**:
   - Executes main block.
   - If `Throw` is caught:
     - Iterates `except` blocks.
     - If matches (currently catch-all or bound), executes handler.
     - Handler execution status updates `action` (e.g., if handler finishes normally, `Throw` is cleared).
   - Executes `finally` block (if present).
     - If `finally` raises/returns/breaks, it overrides previous action.

## Key Structures
- **`ExecAction`**: Structure passed by reference in `Exec` calls.
  - `type`: `None`, `Break`, `Return`, `Throw`.
  - `exceptionValue`: The exception object.
- **`XlangRuntime`**: Stores pending exception (`m_exception`) for propagation across C++ boundaries (e.g. `XObj::Call` interface).

## Testing
- `test2026/exception/1_basic_try.x`: Covers basic raise, try-except, nested blocks, and propagation.
- `test2026/exception/2_early_return.x`: Verifies return statements in try/finally blocks.

## Verification
- **Checkpoints**: `(cpN)` markers in output verify control flow.
- **Error Markers**: `(cp-error)` in output triggers test failure. Used to mark unreachable code paths (e.g. after `raise`).
- **Expectations**: `EXPECT` comments in source code verify specific output lines.
