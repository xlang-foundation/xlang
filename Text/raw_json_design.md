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
    std::string extract_pattern = "", // Template: "data/**/{year:4}/{device}/*"
    std::string filter_expr = ""   // Expression: "year >= 2023 and device == 'A'"
);
```

## 3. Path Extraction Logic ("Smart Glob")

### 3.1. Syntax
The `extract_pattern` argument uses a route-like "Smart Glob" syntax that compiles to Regex internally.

| Token | Description | Regex Equivalent |
| :--- | :--- | :--- |
| `/` | Path Separator (Normalized) | `/` |
| `**` | Recursive Wildcard (Matches any directories) | `.*` |
| `*` | Segment Wildcard (Matches one path segment) | `[^/]*` |
| `{key}` | **Named Capture**: Matches a full segment | `([^/]+)` |
| `{key:N}` | **Fixed Width Capture**: Matches N chars | `(.{N})` |

> [!NOTE] 
> All paths are normalized to use forward slashes (`/`), even on Windows, before matching.

### 3.2. Example
**Pattern**: `root/**/{year:4}/{month:2}/device_{devId}.json`
**Path**: `d:/root/logs/archived/2023/11/device_cam01.json`

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

### 4.1. Grammar & Operators
The `filter_expr` language supports standard SQL-like boolean logic.

#### Logical Operators (In Order of Precedence)
1. `( ... )` : Grouping / Parentheses
2. `AND` : Logical AND (Case-insensitive)
3. `OR` : Logical OR (Case-insensitive)

#### Comparison Operators
| Operator | Description | Example |
| :--- | :--- | :--- |
| `==` or `=` | Equality | `status == 'active'` |
| `!=` | Inequality | `status != 'fail'` |
| `>` | Greater Than | `year > 2022` |
| `>=` | Greater or Equal | `val >= 10.5` |
| `<` | Less Than | `month < 6` |
| `<=` | Less or Equal | `count <= 100` |

#### Literals
- **Numbers**: Integer or Float (e.g., `2023`, `10.5`, `-50`)
- **Strings**: Single or Double quoted (e.g., `'camera'`, `"sensor_01"`)
- **Identifiers**: Metadata keys (e.g., `year`, `devId`)

### 4.2. Resolution & Type Casting
Since all extracted metadata are Strings by default, the engine performs **Automatic Type Coercion**:

1. **Numeric Comparison**: If *any* operand in a comparison is a Number, the metadata string is converted to a Number.
   - `year > 2023` -> `"2025"` becomes `2025.0` -> `true`
2. **String Comparison**: If both operands are Strings, a lexical comparison is used.
   - `devId == 'cam01'` -> `"cam01" == "cam01"` -> `true`

### 4.3. Complex Examples

**Scenario 1: Date Range**
```sql
year >= 2023 AND (month > 6 OR month == 1)
```

**Scenario 2: Specific Device Status**
```sql
(devId == 'cam01' OR devId == 'cam02') AND status != 'maintenance'
```

**Scenario 3: Value Thresholds**
```sql
voltage > 12.5 AND error_count == 0
```

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
