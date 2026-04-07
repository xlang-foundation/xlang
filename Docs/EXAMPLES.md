# XLang™ Examples

The XLang codebase comes with a comprehensive suite of examples and test cases that demonstrate language features and API usage. These acts as living documentation for the interpreter semantics and our internal standard library bindings.

## Locating Examples

Our active test suites are structured within the `test/` and `test2026/` directories, organized by feature area.

### 1. Language Syntax & Structures
Located under `/test/` and `/test2026/general/`, these tests cover core interpreter implementation logic:
- **`list` / `dict` / `set` / `tuple`**: Tests standardizing complex initializations and collection methods safely.
- **`class` / `func`**: Demonstrations of object-oriented structures, lambda patterns, closure trapping, and function decorators (`decor`).
- **`loops` / `if`**: Testing language primitives and block controls.
- **`tensor`**: Specific examples showcasing native inline tensor initialization and expression processing strings.

### 2. Standard Libraries & Built-In Models
The XLang engine includes natively integrated modules, and you can find extensive usage cases:
- **SQLite Database Integration** (`test2026/sqlite/`): Provides examples querying locally, validating dynamic parameter mappings, managing missing columns, and database exceptions.
- **JSON and Formatting** (`test2026/json/`): Serialization and Deserialization patterns of `X::Value` trees.
- **YAML Engine** (`Yaml/test/`): Demonstrates parsing complex documents, attribute overrides, and modifications.
- **Threading Tasks** (`test/task/`): Examples illustrating firing asynchronous thread pool futures and joining outcomes dynamically in script.

### 3. Interoperability & System Glue
As a glue language, testing edge bounds is critical:
- **RPC and IPC**: Check `test/rpc/` for samples initializing listening ports and dispatching remote requests mimicking the `Cantor.Connect` pipeline. 
- **Python Import Engine** (`test2026/python_import/`, `test/pythontest/`): Demonstrates the `PyEng` bridge by importing standardized Python classes and directly evaluating native Python modules without crashing the C++ stack.
- **Computer Vision** (`test/detect/`): Wrappings for simple unified image stream filtering execution architectures.

*Note: As with any actively evolving compiler and ecosystem, you may encounter small breakages in isolated, older `.x` files as we formalize syntax restrictions and expand standard library backward-compatibility.*
