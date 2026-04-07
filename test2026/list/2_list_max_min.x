
print("CHECKPOINTS: cp1, cp2, cp3, cp4, cp5, cp6, cp7")

# --- Integer list ---
nums = [3, 1, 4, 1, 5, 9, 2, 6]
print("(cp1) max of [3,1,4,1,5,9,2,6]:", nums.max())  # expected: 9
print("(cp2) min of [3,1,4,1,5,9,2,6]:", nums.min())  # expected: 1

# --- Float list ---
floats = [3.14, 2.71, 1.41, 1.73]
print("(cp3) max of floats:", floats.max())  # expected: 3.14
print("(cp4) min of floats:", floats.min())  # expected: 1.41

# --- Single-element list ---
single = [42]
print("(cp5) max of [42]:", single.max())  # expected: 42
print("(cp6) min of [42]:", single.min())  # expected: 42

# --- Empty list returns None ---
empty = []
result = empty.max()
print("(cp7) max of empty list:", result)  # expected: None
