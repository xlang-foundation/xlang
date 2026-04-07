# Test `is not` and `is` / `not is` operators
# Each test prints a deterministic EXPECT line so run_tests.py can verify

# Test 1: x is None → True
x = None
if x is None:
    print("T01: x is None: True")
# EXPECT: T01: x is None: True

# Test 2: x is not None → False (x IS None)
if x is not None:
    print("T02: FAIL")
else:
    print("T02: x is not None when None: False")
# EXPECT: T02: x is not None when None: False

# Test 3: y is not None → True (y is an int)
y = 42
if y is not None:
    print("T03: y is not None: True")
# EXPECT: T03: y is not None: True

# Test 4: y is None → False
if y is None:
    print("T04: FAIL")
else:
    print("T04: y is None when int: False")
# EXPECT: T04: y is None when int: False

# Test 5: variable assignment from is not result
result = y is not None
if result:
    print("T05: assigned is not result: True")
# EXPECT: T05: assigned is not result: True

# Test 6: compound with `and`
if x is None and y is not None:
    print("T06: compound is/is not: passed")
# EXPECT: T06: compound is/is not: passed

# Test 7: not is (old syntax) still works
if not x is None:
    print("T07: FAIL")
else:
    print("T07: not is None: correct")
# EXPECT: T07: not is None: correct

# Test 8: string is not None
s = "hello"
if s is not None:
    print("T08: str is not None: True")
# EXPECT: T08: str is not None: True

# Test 9: is not in nested condition
a = 1
b = None
if a is not None:
    if b is None:
        print("T09: nested is/is not: passed")
# EXPECT: T09: nested is/is not: passed

# Test 10: is not used in value expression with print
c = 100
print("T10:", c is not None)
# EXPECT: T10: True

print("Done")
# EXPECT: Done
