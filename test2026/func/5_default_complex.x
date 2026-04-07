
print("CHECKPOINTS: cp1, cp2, cp3")

# cp1: nested function with defaults
def outer(x=5):
    def inner(y=3):
        return x + y
    return inner

fn = outer()       # x uses default 5
r1 = fn()          # y uses default 3
print("(cp1) nested:", r1)   # 8

# cp2: recursion with default stopping condition
def count_down(n, stop=0):
    if n <= stop:
        return 0
    return 1 + count_down(n - 1)

r2 = count_down(3)
print("(cp2) countdown:", r2)   # 3

# cp3: expression-based default (arithmetic)
def shift(x, offset=2+3):
    return x + offset

r3 = shift(10)
print("(cp3) shift:", r3)   # 15
