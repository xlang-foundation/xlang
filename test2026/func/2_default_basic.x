
print("CHECKPOINTS: cp1, cp2, cp3, cp4, cp5")

# cp1: default string param
def greet(name, msg="Hello"):
    print("(cp1)", msg, name)

greet("World")        # uses default msg="Hello"

# cp2: override default
def greet2(name, msg="Hello"):
    print("(cp2)", msg, name)

greet2("World", "Hi")  # overrides default

# cp3: default int
def get_num(x, base=42):
    return x + base

r1 = get_num(0)
print("(cp3) result:", r1)   # 42

# cp4: default float (called with zero args for both defaults)
def get_pi(unused=0, val=3.14):
    return val

r2 = get_pi()
print("(cp4) pi:", r2)       # 3.140000

# cp5: default None
def use_none(x, y=None):
    if y == None:
        return "none_default"
    return "has_value"

r3 = use_none(1)
print("(cp5) none:", r3)     # none_default
