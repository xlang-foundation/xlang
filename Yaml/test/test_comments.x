
from xlang_yaml import yaml

var content = """
# Header Comment
root:
    first: 1 # Inline comment 1
    # Block comment for second
    second: 2
    list:
        - item1 # comment for item1
        - item2
"""

print("Original Content:")
print(content)

var loaded = yaml.loads(content)
var saved = yaml.saves(loaded)

print("\nSaved Content (Comments Check):")
print(saved)


print("\nInternal Comment Check:")
var root = loaded["root"]

# Check Root Comment
var rc = getattr(root, "__root_comment__")
print("Root comment:", rc)

var comments = getattr(root, "__comments__")
var fc = comments["first"]
print("First comment:", fc)

var list_node = root["list"]
var list_comments = getattr(list_node, "__comments__")
var item1_c = list_comments["0"]
print("Item1 comment:", item1_c)
