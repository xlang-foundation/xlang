# Raw JSON Loader Design Specification

## 1. Overview
The `load_raw` function in `xlang.text.json` provides a high-performance mechanism to ingest "raw" (line-delimited) JSON files from a directory structure. Key features include:
- **Recursive Scanning**: Traverses folder hierarchies.
- **Smart Path Extraction**: Extracts metadata (e.g., dates, device IDs) from file paths using a template syntax.
- **Advanced Filtering**: Filters files *before* parsing using a SQL-like conditional expression language.
- **Native Parsing**: Returns a list of fully parsed objects enriched with metadata.

## 2. API Signature

```cpp
// C++ Implementation Signature
X::Value LoadRaw(
    std::string path,              // Root directory to scan
    std::string pattern = "*",     // File glob pattern (e.g. "*.json")
    bool recursive = true,         // Scan subdirectories
    std::string extract_pattern = "", // Template: "data/${year:4}/${device}"
    std::string filter_expr = ""   // Expression: "year >= 2023 and device == 'A'"
);
```

## 3. Path Extraction Logic

### 3.1. Template Syntax
The `extract_pattern` argument defines how to parse the file path for metadata.
- **`${key}`**: Matches any non-separator identifier. Equivalent to Regex `([^/\\\\]+)`.
- **`${key:N}`**: Matches exactly N **alphanumeric characters**. Equivalent to Regex `([a-zA-Z0-9]{N})`.

### 3.2. Example
**Pattern**: `root/${year:4}/${month:2}/device_${devId}.json`
**Path**: `d:/root/2023/11/device_cam01.json`

**Extracted Metadata**:
```json
{
  "year": "2023",
  "month": "11",
  "devId": "cam01"
}
```

## 4. Advanced Filter Expression

To maximize performance, files are filtered *based on metadata* before the file is opened.

### 4.1. Grammar
The `filter_expr` supports standard logical operations and comparisons.

- **Logical Operators**: `AND`, `OR` (case-insensitive), Parentheses `(`, `)`
- **Comparison Operators**: `>`, `>=`, `<`, `<=`, `==` (or `=`), `!=`
- **Values**:
    - **Numbers**: `2023`, `10.5`
    - **Strings**: `'text'`, `"text"` (Quotes required for string literals)
    - **Identifiers**: `year`, `devId` (Reference extracted metadata keys)

### 4.2. Resolution & Type Casting
Since all extracted metadata are Strings by default:
1. **Numeric Comparison**: If the operand is a Number (e.g., `year > 2023`), the metadata value is implicitly cast to a Number for the comparison.
2. **String Comparison**: If the operand is a String (e.g., `devId == 'cam01'`), a lexical comparison is performed.

### 4.3. Examples
- **Time Range**: `year > 2022 and (month >= 10 or month == 1)`
- **Device Filter**: `devId == 'cam01' or devId == 'cam99'`
- **Complex**: `(year == 2023 and month > 5) and status != 'debug'`

## 5. Return Format

The function returns a **List** of **Objects**.

```json
[
  {
    // The parsed content of the line
    "id": 101,
    "value": 55,

    // Meta-fields injected by the loader
    "_meta": {
      "year": "2023",
      "month": "11",
      "devId": "cam01"
    },
    "_file": "d:/root/2023/11/device_cam01.json"
  }
]
```

## 6. Implementation Strategy (C++)

1.  **Tokenizer**: Breaks `filter_expr` into tokens (IDENTIFIER, NUMBER, STRING, OP_GT, OP_AND, etc.).
2.  **Shunting-Yard Parser**: Converts infix tokens to Reverse Polish Notation (RPN) for efficient evaluation.
3.  **Evaluator**:
    - Iterates through RPN stack for each file's metadata.
    - Performs fast casting and comparison.
    - Returns `boolean` result.
4.  **Integration**:
    - `Folder::Scan` -> `Regex Match` -> `Extract Meta` -> `Evaluator(Meta)` -> `Parse JSON` -> `Result`.
