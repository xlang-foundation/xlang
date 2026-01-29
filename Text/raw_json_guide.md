# XLang JSON Raw Loader (`json.load_raw`) User Guide

The `json.load_raw` function allows you to efficiently load, parse, and filter large datasets of "Raw JSON" (line-delimited JSON) files scattered across directory structures. It combines high-performance recursive scanning with SQL-like filtering on file metadata.

---

## 1. Quick Start

```python
import json

# Load all .jsonl files in a folder structure
# Pattern: Root -> Year -> Month -> Data Files
records = json.load_raw(
    path="d:/logs", 
    extract_pattern="**/{year:4}/{month:2}/**",
    filter_expr="year >= 2023 AND month == 11"
)

# Accessing Data
for rec in records:
    print(f"Data: {rec['value']}")       # Record content
    print(f"File: {rec['_file']}")       # Source file path
    print(f"Year: {rec['_meta']['year']}") # Extracted metadata
```

---

## 2. API Reference

```python
json.load_raw(
    path: str,              # Root directory to start scanning
    pattern: str = "*",     # File glob pattern (e.g., "*.json", "*.log")
    recursive: bool = True, # Whether to search subdirectories
    extract_pattern: str,   # "Smart Glob" pattern for metadata extraction
    filter_expr: str        # Logic expression to filter files
) -> list[dict]
```

---

## 3. Path Extraction ("Smart Glob")

The `extract_pattern` uses a robust, route-like syntax to capture metadata from file paths. This metadata is stored in the `_meta` dictionary of each record and used for filtering.

### 3.1. Syntax Reference

| Syntax Token | Description | Regex Equivalent | Notes |
| :--- | :--- | :--- | :--- |
| **`/`** | **Path Separator** | `/` | All paths are normalized to `/`. <br>Use `/` even on Windows. |
| **`**`** | **Recursive Wildcard** | `.*` | Matches zero or more directories/chars. <br>Use between folders like `root/**/data`. |
| **`*`** | **Segment Wildcard** | `[^/]*` | Matches content within a *single* folder/file segment. |
| **`{name}`** | **Named Capture** | `([^/]+)` | Captures a full path segment into `_meta["name"]`. |
| **`{name:N}`** | **Fixed-Width Capture** | `(.{N})` | Captures exactly **N** characters into `_meta["name"]`. <br>Useful for strict dates (`{year:4}`). |

### 3.2. Examples

**Scenario 1: Standard Logs**
- **File**: `d:/logs/2023/server-01.log`
- **Pattern**: `**/{year:4}/{server}.log`
- **Result**: `{"year": "2023", "server": "server-01"}`

**Scenario 2: Deeply Nested with ID**
- **File**: `data/us-east/sensors/grp_A/id_5500/data.json`
- **Pattern**: `**/grp_{group}/id_{id}/**`
- **Result**: `{"group": "A", "id": "5500"}`

**Scenario 3: Fixed Width Date**
- **File**: `archive/20231201_data.json`
- **Pattern**: `**/{year:4}{month:2}{day:2}_data.json`
- **Result**: `{"year": "2023", "month": "12", "day": "01"}`

---

## 4. Filter Expressions (`filter_expr`)

Filter files **before** they are opened by evaluating logical expressions against the extracted metadata. This provides significant performance benefits.

### 4.1. Grammar & Operators

**Precedence**: `( )` > `AND` > `OR`

| Operator | Type | Description |
| :--- | :--- | :--- |
| **`AND`** | Logical | Returns true if **both** sides are true. (Case-insensitive) |
| **`OR`** | Logical | Returns true if **either** side is true. (Case-insensitive) |
| **`==`**, **`=`** | Comparison | Strict equality check. |
| **`!=`** | Comparison | Inequality check. |
| **`>`**, **`>=`** | Comparison | Greater than / Greater or Equal. |
| **`<`**, **`<=`** | Comparison | Less than / Less or Equal. |

### 4.2. Values and Types

- **Variables**: Use the keys extracted in your `extract_pattern` (e.g., `year`, `id`).
- **Strings**: Must be enclosed in single (`'`) or double (`"`) quotes.
- **Numbers**: Integers (`2023`) or Floats (`10.5`).

### 4.3. Automatic Type Casting

Extracted metadata is always a **String**. However, the filter engine performs **Automatic Type Coercion**:

1.  **Numeric Context**: If you compare a variable to a **Number**, the variable is converted to a number.
    - `year > 2022` : `year` string "2023" becomes number `2023.0`. -> **True**
    
2.  **String Context**: If you compare a variable to a **String**, a standard dictionary (lexical) comparison is used.
    - `status == 'active'` : Strict string equality. -> **True**

### 4.4. Complex Filter Examples

**Time Window**
```sql
year >= 2023 AND (month > 6 OR month == 1)
```

**Specific Items excluding status**
```sql
(device == 'camera_01' OR device == 'sensor_X') AND status != 'maintenance'
```

**Numeric Thresholds**
```sql
voltage <= 3.3 AND region == 'us-west'
```
