
# Test Filesystem APIs

from xlang_os import fs

folder = fs.Folder("/tmp_test_xlang_fs")

print("Testing Folder ops...")
# 1. Prepare
if folder.Exists():
    print("Cleaning up old test folder...")
    folder.removedirs(folder.Path)

# 2. mkdir
print("Making dirs...")
folder.makedirs(folder.Path)
if not folder.Exists():
    print("ERROR: makedirs failed")
else:
    print("makedirs success")

# 3. File ops
print("Testing File ops...")
f_path = folder.BuildPath("test.txt")
f = fs.File(f_path, "w")
f.write("Hello\nWorld")
f.close()

f = fs.File(f_path, "r")
c = f.read(-1)
print("File content:", c)
f.close()

if c != "Hello\nWorld":
    print("ERROR: File write/read failed")

# 4. Glob
print("Testing Glob...")
# Create subdir
sub = folder.BuildPath("sub")
fs.Folder(sub).makedirs(sub)
f2 = file(folder.BuildPath("sub/test2.txt"), "w")
f2.write("sub")
f2.close()

files = folder.glob("**/*.txt")
print("Glob **/*.txt results:", files)
# Expected: test.txt and sub/test2.txt (relative paths)

# 5. is_under
print("Testing is_under...")
if folder.is_under(folder.BuildPath("sub/test2.txt")):
    print("is_under check 1 passed")
else:
    print("ERROR: is_under check 1 failed")

if not folder.is_under("/etc/passwd"):
    print("is_under check 2 passed")
else:
    print("ERROR: is_under check 2 failed")

# 6. Walk
print("Testing Walk...")
# walk requires topdown arg because of AddFunc<1> binding
w = folder.walk(True)
# My walk implementation currently returns a List, not a generator yielding tuples
# Emulating standard python walk is hard without generator support in current simple bind
print("Walk result:", w)

# Cleanup
print("Cleanup...")
# folder.removedirs(folder.Path) 
# Keep for inspection
print("Done.")
