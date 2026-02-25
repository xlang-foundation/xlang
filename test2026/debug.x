
import json
print("Loaded json module")
try:
    # Attempt to access load_raw
    print("Has load_raw:", json.load_raw)
    files = json.load_raw(".")
    print("Call success, count:", len(files))
except Exception as e:
    print("Error:", e)
