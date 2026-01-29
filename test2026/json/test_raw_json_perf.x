
import json

root_path = "d:/test/raw_json_perf"
total_records = 10000

print("Calling json.load_raw...")
try:
    p_init = "**/{grp}/**" # or "**/sub_{grp}/**" ? Perf test paths are .../sub_N/file_N.jsonl
    # User's perf test path structure: root/sub_N/file_N.jsonl
    # Regex was: .*sub_([a-zA-Z0-9_]+)/.* 
    # Smart Glob: **/sub_{grp}/** 
    # match: sub_0 matches sub_{grp} ? 
    # {grp} = ([^/]+).  sub_([^/]+). Yes.
    
    p_init = "**/sub_{grp}/**"
    recs = json.load_raw(root_path, recursive=True, extract_pattern=p_init)
    count = len(recs)
    print("Loaded records: " + str(count))

    if count != total_records:
        print("FAIL: Count Expected mismatch")
    else:
        print("PASS: Count matches")

    p = "**/sub_{grp}/**"
    recs = json.load_raw(root_path, recursive=True, extract_pattern=p)
    # ...
    p_filter = "**/sub_{grp}/**"
    recs_filtered = json.load_raw(root_path, recursive=True, extract_pattern=p_filter, filter_expr="grp >= 5")
    
    f_count = len(recs_filtered)
    print("Filtered count: " + str(f_count))
    
    if f_count != 5000:
        print("FILTER FAIL: Expected 5000, got " + str(f_count))
    else:
        print("FILTER PASS")

except Exception as e:
    print("EXCEPTION: " + str(e))
