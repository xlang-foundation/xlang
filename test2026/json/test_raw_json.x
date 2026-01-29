
# EXPECT: Found 3 records
# EXPECT: Extraction Verified: 2023/01
# EXPECT: Filter Verified: Only 2023 files

from xlang_os import fs
import json

root_path = "d:/test/raw_json_test"
root_folder = fs.Folder(root_path)

if root_folder.Exists():
    # Cleanup: remove old folder
    # Note: removedirs might not be recursive for non-empty? 
    # Let's hope removedirs works or just ignore if fail (makedirs will fix)
    # But to be clean, let's try remove. 
    # If not implemented recursively in xlang yet, we might have issues. 
    # For now, assume it works or we overwrite.
    root_folder.removedirs(root_path)

# Create Dirs
path_23 = root_folder.BuildPath("2023/01")
fs.Folder(path_23).makedirs(path_23)

path_22 = root_folder.BuildPath("2022/12")
fs.Folder(path_22).makedirs(path_22)

# Create Files
f1_path = fs.Folder(path_23).BuildPath("data.jsonl")
f1 = fs.File(f1_path, "w")
f1.write('''{"id": 1, "val": "A"}
{"id": 2, "val": "B"}''')
f1.close()

f2_path = fs.Folder(path_22).BuildPath("old.jsonl")
f2 = fs.File(f2_path, "w")
f2.write('''{"id": 0, "val": "Old"}''')
f2.close()

# Debug: Verify files exist
f1_check = fs.File(f1_path, "r")
print(f"File 1 size: {f1_check.size()}")
f1_check.close()


# Test 1: Recursive Load
files = json.load_raw(root_path, recursive=True)
print(f"Found {len(files)} records") 

# Test 2: Extraction (New "Smart Glob" Syntax)
recs = json.load_raw(root_path, extract_pattern="**/{year:4}/{month:2}/**", pattern="*.jsonl")
matched = 0
for r in recs:
    meta = r["_meta"]
    if meta["year"] == "2023" and meta["month"] == "01":
        matched += 1
if matched == 2:
    print("Extraction Verified: 2023/01")

# Test 3: Filter Expression
# filter_expr="year >= 2023"
# Note: extracted 'year' is string "2023". Filter converts to number if comparing with number.
recs2 = json.load_raw(root_path, 
    extract_pattern="**/{year:4}/{month:2}/**", 
    filter_expr="year >= 2023") 

filtered_count = len(recs2)
if filtered_count == 2:
    print("Filter Verified: Only 2023 files")
else:
    print(f"Filter Failed: Got {filtered_count}")

# Test 4: Partial Segment Matching
# Path: path1/str1972/11.jsonl
path_partial = root_folder.BuildPath("path1/str1972")
fs.Folder(path_partial).makedirs(path_partial)

f3_path = fs.Folder(path_partial).BuildPath("11.jsonl")
f3 = fs.File(f3_path, "w")
f3.write('''{"id": 3, "val": "Partial"}''')
f3.close()

recs3 = json.load_raw(root_folder.BuildPath("path1"), 
    extract_pattern="**/str{year:4}/**", 
    pattern="*.jsonl")

if len(recs3) == 1 and recs3[0]["_meta"]["year"] == "1972":
    print("Partial Segment Match Verified: 1972")
else:
    # check valid 
    print(f"Partial Segment Failed. Len: {len(recs3)}")
    if len(recs3) > 0:
        print(f"Meta: {recs3[0]['_meta']}")

# Test 5: Normalization (Mixed Separators)
# Input path uses backslashes, pattern uses forward slashes
# This confirms the engine normalizes the input path to '/' before matching.
path_mixed = root_folder.BuildPath("mixed_test")
fs.Folder(path_mixed).makedirs(path_mixed)

f4_path = path_mixed + "\\data_mixed.jsonl"
# Create file manually with backslash path if possible, or just use fs
f4 = fs.File(f4_path, "w")
f4.write('''{"id": 4, "val": "Mixed"}''')
f4.close()

# Pass path with backslashes to load_raw
root_mixed_bs = root_path.replace("/", "\\") + "\\mixed_test"
recs4 = json.load_raw(root_mixed_bs, 
    extract_pattern="**/mixed_test/**", 
    pattern="*.jsonl")

if len(recs4) == 1:
    print("Normalization Verified: Backslashes matches forward slash pattern")
else:
    print(f"Normalization Failed. Got {len(recs4)}")


# Cleanup
# root_folder.removedirs(root_path)
