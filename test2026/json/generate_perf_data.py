
import os
import shutil

root_path = "d:/test/raw_json_perf"

if os.path.exists(root_path):
    print("Cleaning up old dir...")
    try:
        shutil.rmtree(root_path)
    except:
        pass

if not os.path.exists(root_path):
    os.makedirs(root_path)

file_count = 1000
rows_per_file = 10
total_records = file_count * rows_per_file

print(f"Generating {file_count} files ({total_records} records)...")

content = ""
for i in range(rows_per_file):
    content += '{"id": ' + str(i) + ', "val": "keks_perf_test", "data": [1,2,3,4,5]}\n'

for i in range(file_count):
    folder_idx = int(i / 100)
    sub_path = os.path.join(root_path, "sub_" + str(folder_idx))
    if not os.path.exists(sub_path):
        os.makedirs(sub_path)
    f_path = os.path.join(sub_path, "file_" + str(i) + ".jsonl")
    with open(f_path, "w") as f:
        f.write(content)

print("Generation Done. Files created.")
