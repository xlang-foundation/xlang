# XLang `json.load_raw` Specification

The `json.load_raw` function provides high-performance loading, filtering, and sorting of line-delimited JSON files (JSONL) distributed across directory structures. It supports "Smart Glob" path extraction, hybrid metadata/content filtering, and in-memory sorting/pagination.

## 1. Function Signature

```python
json.load_raw(
    path: str,
    pattern: str = "*",
    recursive: bool = True,
    extract_pattern: str = "",
    filter_expr: str = "",
    sort: Union[str, list[str], list[dict]] = None,
    reverse: bool = False,
    numeric: bool = False,
    offset: int = 0,
    limit: int = -1,
    filter_meta: dict = None
) -> list[dict]
```

## 2. Parameter Definitions

| Parameter | Type | Description |
| :--- | :--- | :--- |
| **`path`** | `str` | Root directory path to start scanning. |
| **`pattern`** | `str` | Filename glob pattern (e.g., `*.json`, `data_*.log`). Default: `*`. |
| **`recursive`** | `bool` | If `True`, scans subdirectories recursively. Default: `True`. |
| **`extract_pattern`** | `str` | "Smart Glob" pattern to extract metadata from file paths (e.g., `**/{year}/{month}/**`). Extracted values are stored in `_meta`. |
| **`filter_expr`** | `str` | SQL-like expression to filter records. Supports `AND`, `OR`, `==`, `>`, `<`, etc. |
| **`sort`** | `str` \| `list` | Sorting specification (see Section 3). |
| **`reverse`** | `bool` | Global flag to reverse sort order. Default: `False`. |
| **`numeric`** | `bool` | Default for "Smart Numeric" sorting (strips units). Applies to all keys unless overridden in `sort`. Default: `False`. |
| **`offset`** | `int` | Number of records to skip (applied *after* sorting). Default: `0`. |
| **`limit`** | `int` | Maximum number of records to return. `-1` means no limit. Default: `-1`. |

## 3. Sorting Specification

Sorting happens in-memory after all matching records are loaded and filtered. The `sort` parameter supports three formats:

### 3.1. Single Key (String)
Sorts by a single field.
```python
json.load_raw(..., sort="timestamp")
```

### 3.2. Multi-Key (List of Strings)
Sorts by multiple fields in order of priority.
```python
# Sort by Category, then by Rank
json.load_raw(..., sort=["category", "rank"])
```

### 3.3. Detailed Configuration (List of Dictionaries)
Allows per-field configuration for `reverse` and `numeric` handling.
```python
json.load_raw(..., sort=[
    {"field": "category", "reverse": False},           # A-Z
    {"field": "voltage", "numeric": True},             # Numeric (5V < 20V)
    {"field": "timestamp", "reverse": True}            # Newest first
])
```

### 3.4. Global vs. Local Flags (Dual Scope)
The `reverse` and `numeric` parameters can be used in two ways:
1.  **Global (Root Parameter)**: Applies to **ALL** fields in the sort.
2.  **Local (Inside Dict)**: Overrides the global setting for that specific field.

**Example: Mixed Numeric/Lexical Sort**
```python
# Global numeric=False (default)
json.load_raw(...,
    sort=[
        {"field": "category"},                 # Uses Global (Lexical)
        {"field": "voltage", "numeric": True}  # Overrides to Numeric
    ]
)
```

### 3.5. Global Flags
- **`reverse`**: If `True`, reverses the order of ALL sort keys (unless overridden).
- **`numeric`**: If `True`, enables smart numeric parsing for ALL sort keys (unless overridden).

## 4. Hybrid Filtering (`filter_expr`)

The filter engine automatically determines the most efficient scope for your expression:

1.  **File-Level (Optimization)**: If `filter_expr` only references variables found in `extract_pattern` (e.g., `year`, `month`), entire files are skipped without opening them.
2.  **Row-Level (Content)**: If `filter_expr` references variables inside the JSON content (e.g., `voltage`, `status`), the file is opened and matched line-by-line.

**Example**:
```python
# 'year' is extracted from path (File-Level)
# 'voltage' is inside JSON (Row-Level)
json.load_raw(..., 
    extract_pattern="**/{year}/**",
    filter_expr="year == '2023' AND voltage > 12.0"
)
```

## 5. Usage Examples

### Example 1: Basic Load with Sort
Load all logs, sort by ID descending.
```python
data = json.load_raw(
    "d:/data",
    recursive=True,
    sort="id",
    reverse=True
)
```

### Example 2: Pagination
Get the "second page" of 20 items.
```python
data = json.load_raw(
    "d:/data",
    sort="timestamp",
    offset=20,
    limit=20
)
```

### Example 3: Smart Numeric Sort
Sort string values that contain units (e.g., "100mV", "5V").
```python
# Correctly sorts: "100mV", "5V", "10V" (0.1, 5.0, 10.0)
data = json.load_raw(
    "d:/data",
    sort="voltage_str",
    numeric=True
)
```

### Example 4: Complex Multi-Key Sort
Sort by **Category** (A-Z), then by **Voltage** (High-to-Low, numeric).
```python
data = json.load_raw(
    "d:/data",
    sort=[
        {"field": "category"},
        {"field": "voltage", "numeric": True, "reverse": True}
    ]
)
```
