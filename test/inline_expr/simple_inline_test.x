# Smoke test for inline expressions

# Test 1: inline if (ternary)
x = 10
r1 = "yes" if x > 5 else "no"
print(r1)

# Test 2: block if (must still work)
if x > 5:
    print("block works")

# Test 3: inline for (list comprehension)
squares = [i*i for i in range(5)]
print(squares)

# Test 4: inline for with filter
evens = [i for i in range(10) if i % 2 == 0]
print(evens)

# Test 5: block for (must still work)
for i in range(3):
    print(i)
print("done")