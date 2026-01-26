# Comprehensive Tuple Test for XLang
# Tests tuple creation, unpacking, indexing, and multi-value returns

print("CHECKPOINTS: cp1, cp2, cp3, cp4, cp5, cp6, cp7, cp8, cp9, cp10, cp11, cp12, cp13, cp14, cp15, cp16, cp17, cp18")

# Test 1: Basic tuple creation with parentheses
t = (1, 2, 3)
print(f"(cp1) tuple creation: {t}")
print("(cp1-check) tuple creation: (1, 2, 3)")

# Test 2: Tuple creation without parentheses (implicit)
t2 = 10, 20, 30
print(f"(cp2) implicit tuple: {t2}")
print("(cp2-check) implicit tuple: (10, 20, 30)")

# Test 3: Single element tuple (with trailing comma)
t3 = (42,)
print(f"(cp3) single element: {t3}")
print("(cp3-check) single element: (42,)")

# Test 4: Tuple indexing - positive
t = ("apple", "banana", "cherry")
item = t[0]
print(f"(cp4) index [0]: {item}")
print("(cp4-check) index [0]: apple")

# Test 5: Tuple indexing - negative
t = ("apple", "banana", "cherry")
item = t[-1]
print(f"(cp5) index [-1]: {item}")
print("(cp5-check) index [-1]: cherry")

# Test 6: Tuple unpacking - basic
t = (100, 200, 300)
a, b, c = t
print(f"(cp6) unpack: a={a}, b={b}, c={c}")
print("(cp6-check) unpack: a=100, b=200, c=300")

# Test 7: Tuple unpacking - direct
x, y, z = 1, 2, 3
print(f"(cp7) direct unpack: x={x}, y={y}, z={z}")
print("(cp7-check) direct unpack: x=1, y=2, z=3")

# Test 8: Swapping values using tuples
a = 10
b = 20
a, b = b, a
print(f"(cp8) swap: a={a}, b={b}")
print("(cp8-check) swap: a=20, b=10")

# Test 9: Function returning tuple
def get_coordinates():
    return 10, 20

coords = get_coordinates()
print(f"(cp9) function return tuple: {coords}")
print("(cp9-check) function return tuple: (10, 20)")

# Test 10: Function returning tuple - unpacked
def get_point():
    return 5, 15

x, y = get_point()
print(f"(cp10) unpack return: x={x}, y={y}")
print("(cp10-check) unpack return: x=5, y=15")

# Test 11: Tuple with mixed types
mixed = (1, "hello", 3.14, True)
print(f"(cp11) mixed types: {mixed}")
print("(cp11-check) mixed types: (1, hello, 3.14, True)")

# Test 12: Nested tuples
nested = (1, (2, 3), 4)
print(f"(cp12) nested: {nested}")
print("(cp12-check) nested: (1, (2, 3), 4)")

# Test 13: Accessing nested tuple elements
nested = (1, (2, 3), 4)
inner = nested[1]
val = inner[0]
print(f"(cp13) nested access: {val}")
print("(cp13-check) nested access: 2")

# Test 14: Tuple length
t = (10, 20, 30, 40)
length = len(t)
print(f"(cp14) length: {length}")
print("(cp14-check) length: 4")

# Test 15: Tuple slicing
t = (1, 2, 3, 4, 5)
sub = t[1:4]
print(f"(cp15) slice [1:4]: {sub}")
print("(cp15-check) slice [1:4]: (2, 3, 4)")

# Test 16: Tuple concatenation
t1 = (1, 2)
t2 = (3, 4)
combined = t1 + t2
print(f"(cp16) concatenation: {combined}")
print("(cp16-check) concatenation: (1, 2, 3, 4)")

# Test 17: Tuple repetition
t = (1, 2) * 3
print(f"(cp17) repetition: {t}")
print("(cp17-check) repetition: (1, 2, 1, 2, 1, 2)")

# Test 18: Using tuple in for loop
t = (10, 20, 30)
sum = 0
for val in t:
    sum += val
print(f"(cp18) tuple iteration sum: {sum}")
print("(cp18-check) tuple iteration sum: 60")
