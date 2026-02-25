
print("CHECKPOINTS: cp1, cp2, cp3, cp4")

# Verify that caller-provided args always override defaults

def calc(a, b=10, c=100):
    return a + b + c

# cp1: all defaults used for b and c
r1 = calc(1)
print("(cp1) calc(1):", r1)       # 111

# cp2: override b, c uses default
r2 = calc(1, 2)
print("(cp2) calc(1,2):", r2)     # 103

# cp3: override both b and c
r3 = calc(1, 2, 3)
print("(cp3) calc(1,2,3):", r3)   # 6

# cp4: a=0, use all defaults
r4 = calc(0)
print("(cp4) calc(0):", r4)       # 110
