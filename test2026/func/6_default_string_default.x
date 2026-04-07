
print("CHECKPOINTS: cp1, cp2, cp3")

# cp1: string concatenation with default
def join_words(a, b="world"):
    return a + " " + b

r1 = join_words("hello")
print("(cp1) join:", r1)    # hello world

# cp2: boolean default - use default False
def is_on(name, enabled=False):
    if enabled:
        return name + ":ON"
    return name + ":OFF"

r2 = is_on("light")
print("(cp2) bool:", r2)    # light:OFF

# cp3: call with zero args (all defaults)
def all_defaults(x=1, y=2, z=3):
    return x + y + z

r3 = all_defaults()
print("(cp3) all defaults:", r3)   # 6
