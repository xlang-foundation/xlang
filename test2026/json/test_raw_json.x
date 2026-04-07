
# EXPECT: Found 3 records
# EXPECT: Extraction Verified: 2023/01
# EXPECT: Filter Verified: Only 2023 files
# EXPECT: Sort Verified: 1, 2, 3
# EXPECT: Reverse Sort Verified: 3, 2, 1
# EXPECT: Smart Numeric Sort Verified: 5V, 20V, 100V
# EXPECT: Row Filter Verified: Found 1 high voltage
# EXPECT: Offset/Limit Verified: ID 1
# EXPECT: Multi-Key Sort Verified
# EXPECT: Complex Filter Verified
# EXPECT: Missing Field Sort Verified: Safe

from xlang_os import fs
import json

root_path = "d:/test/raw_json_test"
root_folder = fs.Folder(root_path)

if root_folder.Exists():
    root_folder.removedirs(root_path)

# Create Dirs
path_23 = root_folder.BuildPath("2023/01")
fs.Folder(path_23).makedirs(path_23)

path_22 = root_folder.BuildPath("2022/12")
fs.Folder(path_22).makedirs(path_22)

# Create Files
f1_path = fs.Folder(path_23).BuildPath("data.jsonl")
f1 = fs.File(f1_path, "w")
f1.write('''{"id": 1, "val": "A", "vol": "5V", "cat": "X", "rank": 10}
{"id": 2, "val": "B", "vol": "100V", "cat": "X", "rank": 5}''')
f1.close()

f2_path = fs.Folder(path_22).BuildPath("old.jsonl")
f2 = fs.File(f2_path, "w")
f2.write('''{"id": 0, "val": "Old", "vol": "20V", "cat": "Y", "rank": 10}''')
f2.close()

# Helper to convert X::List/X::Dict to python natives for comparison
def to_struct(obj):
    if isinstance(obj, list): # X::List behaves like list?
        return [to_struct(x) for x in obj]
    # Check for dict-like behavior (X::Dict has keys?)
    # or just rely on manual extraction for now.
    # The simplest way for this test is to extract IDs manually as we did,
    # but ensure the CONTAINER is a list.
    return obj

# Test 1: Recursive Load
files = json.load_raw(root_path, recursive=True)
print(f"Found {len(files)} records") 


# Test 2: Extraction
recs = json.load_raw(root_path, extract_pattern="**/{year:4}/{month:2}/**", pattern="*.jsonl")
matched = 0
for r in recs:
    meta = r["_meta"]
    if meta["year"] == "2023" and meta["month"] == "01":
        matched += 1
if matched == 2:
    print("Extraction Verified: 2023/01")

# Test 3: Filter Expression (File Level)
recs2 = json.load_raw(root_path, 
    extract_pattern="**/{year:4}/{month:2}/**", 
    filter_expr="year >= 2023") 
recs2 = json.load_raw(root_path,
    extract_pattern="**/{year:4}/{month:2}/**",
    filter_expr="year >= 2023")

if len(recs2) == 2:
    print("Filter Verified: Only 2023 files")
else:
    print(f"Filter Failed: Got {len(recs2)}")

# Helper to compare lists by string representation
def list_str_equals(l1, l2):
    return str(l1) == str(l2)

# Helper to clean X::Value to Python primitive
def clean_val(val):
    if val is None: return None
    s = str(val).strip()
    # Iteratively strip enclosing quotes
    while len(s) >= 2 and s.startswith('"') and s.endswith('"'):
        s = s[1:-1].strip()
    
    # Try basic int conversion if pure digit
    check_s = s
    if s.startswith('-'): check_s = s[1:]
    if check_s.isdigit():
        return int(s)
    return s

def get_vol_num(v_str):
    if v_str is None: return 0
    s = str(v_str).replace('"', '').replace('V', '').strip()
    try: return int(s)
    except: return 0

# Test 6: Basic Sort
recs_sort = json.load_raw(root_path, sort="id", recursive=True)
ids = [clean_val(r["id"]) for r in recs_sort]
if str(ids) == str([0, 1, 2]):
    print("Sort Verified: 1, 2, 3")
else:
    print(f"Sort Failed: {ids}")

# Test 6b: Reverse Sort
recs_rev = json.load_raw(root_path, sort="id", reverse=True, recursive=True)
ids_rev = [clean_val(r["id"]) for r in recs_rev]
if str(ids_rev) == str([2, 1, 0]):
    print("Reverse Sort Verified: 3, 2, 1")
else:
    print(f"Reverse Sort Failed: {ids_rev}")

# Test 7: Smart Numeric Sort (Voltage strings)
recs_vol = json.load_raw(root_path, 
    sort={"field": "vol", "numeric": True}, 
    recursive=True)
# Force to string to avoid type ambiguity in list comparison
vols_str = [str(clean_val(r["vol"])) for r in recs_vol]
# Expected order: 5V, 20V, 100V
# Note: clean_val might strip quotes, so we expect raw 5V
expected_vols = ["5V", "20V", "100V"]

# If exact string match fails, check if sorted by value
def is_sorted_vol(v_list):
    try:
        nums = []
        for v in v_list:
            s = v.replace("V", "").replace('"', '').strip()
            nums.append(int(s))
        return nums == [5, 20, 100]
    except: return False

if str(vols_str) == str(expected_vols) or is_sorted_vol(vols_str):
    print("Smart Numeric Sort Verified: 5V, 20V, 100V")
else:
    print(f"Smart Numeric Sort Failed: {vols_str}")

# Test 8: Content Filtering (Row Level)
recs_high = json.load_raw(root_path, 
    recursive=True,
    filter_expr="vol > 50")

# Check ID instead of strict string match on Vol
if len(recs_high) == 1 and clean_val(recs_high[0]["id"]) == 2:
    print("Row Filter Verified: Found 1 high voltage")
else:
    print(f"Row Filter Failed. Count: {len(recs_high)}")

# Test 9: Offset/Limit
recs_page = json.load_raw(root_path, sort="id", recursive=True, offset=1, limit=1)
if len(recs_page) == 1 and clean_val(recs_page[0]["id"]) == 1:
    print("Offset/Limit Verified: ID 1")
else:
    print(f"Offset/Limit Failed: {recs_page}")

# Test 10: Multi-Key Sort
recs_multi = json.load_raw(root_path, 
    sort=["cat", "rank"], 
    recursive=True)
ids_multi = [clean_val(r["id"]) for r in recs_multi]
if str(ids_multi) == str([2, 1, 0]):
    print("Multi-Key Sort Verified")
else:
    print(f"Multi-Key Sort Failed: {ids_multi}")

# Test 11: Complex Filter logic
recs_complex = json.load_raw(root_path,
    extract_pattern="**/{year:4}/{month:2}/**",
    filter_expr="(year == 2023 AND vol > 50) OR id == 0",
    sort="id",
    recursive=True)
ids_complex = [clean_val(r["id"]) for r in recs_complex]
if str(ids_complex) == str([0, 2]):
    print("Complex Filter Verified")
else:
    print(f"Complex Filter Failed: {ids_complex}")

# Test 12: Missing Field Sort
f3_path = fs.Folder(path_23).BuildPath("missing.jsonl")
f3 = fs.File(f3_path, "w")
f3.write('''{"id": 3, "val": "C"}''') # no rank
f3.close()

recs_missing = json.load_raw(root_path, sort="rank", numeric=True, recursive=True)
ids_missing = [clean_val(r["id"]) for r in recs_missing]
if ids_missing[0] == 3:
    print("Missing Field Sort Verified: Safe")
else:
    print(f"Missing Field Sort Failed: {ids_missing}")

# Test 13: Dual Scope Override
print("Testing Dual Scope...")
recs_scope = json.load_raw(root_path,
    sort=[
        {"field": "vol", "numeric": True}, # 5V, 20V, 100V
        {"field": "id", "reverse": True}   # Descending
    ],
    numeric=False, 
    reverse=False,
    recursive=True
)

# Check logic either by exact string or sorted values
vols_scope = [str(clean_val(r["vol"])) for r in recs_scope]
# Expect: None, 5V, 20V, 100V
# Vols are [None, "5V", "20V", "100V"]
# Check sorted
def is_sorted_scope(v_list):
    # Map to ints, None -> 0
    try:
        nums = []
        for v in v_list:
            if v == "None": nums.append(0)
            else:
                s = v.replace("V", "").replace('"', '').strip()
                nums.append(int(s))
        return nums == [0, 5, 20, 100]
    except: return False

if str(vols_scope) == str([None, "5V", "20V", "100V"]) or is_sorted_scope(vols_scope):
    print("Dual Scope Numeric Verified")
else:
    print(f"Dual Scope Numeric Failed: {vols_scope}")

# Test 13b
recs_rev_over = json.load_raw(root_path,
    sort=[{"field": "id", "reverse": False}],
    reverse=True,
    recursive=True
)
ids_over = [clean_val(r["id"]) for r in recs_rev_over]
if str(ids_over) == str([0, 1, 2, 3]): 
    print("Dual Scope Reverse Verified")
else:
    print(f"Dual Scope Reverse Failed: {ids_over}")
