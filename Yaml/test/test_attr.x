
var d = {
    "key": "value"
}

print("Original Dict:", d)

# Set attribute
print("Setting __yaml_comment__...")
d.setattr("__yaml_comment__", "This is a comment")

# Get attribute
try:
    var c = getattr(d, "__yaml_comment__")
    print("Retrieved comment:", c)
except:
    print("Failed to retrieve comment via getattr")

# Check if setattr persists
print("Dict after setattr (internal check):", d)
