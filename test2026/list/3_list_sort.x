
print("CHECKPOINTS: cp1, cp2, cp3, cp4, cp5, cp6")

# --- Default ascending sort (integers) ---
nums = [5, 2, 8, 1, 9, 3]
nums.sort()
print("(cp1) sorted asc:", nums)   # expected: [1, 2, 3, 5, 8, 9]

# --- Descending sort ---
nums2 = [5, 2, 8, 1, 9, 3]
nums2.sort(False)
print("(cp2) sorted desc:", nums2)  # expected: [9, 8, 5, 3, 2, 1]

# --- Already sorted list ---
asc = [1, 2, 3, 4, 5]
asc.sort()
print("(cp3) already sorted:", asc)  # expected: [1, 2, 3, 4, 5]

# --- Single element ---
single = [42]
single.sort()
print("(cp4) single element:", single)  # expected: [42]

# --- Float list ---
floats = [3.14, 1.41, 2.71, 1.73]
floats.sort()
print("(cp5) float asc:", floats)   # expected: [1.41, 1.73, 2.71, 3.14]

# --- Custom sortfunc: sort by absolute value (ascending) ---
data = [-5, 3, -1, 4, -2]
def abs_cmp(a, b):
    return abs(a) - abs(b)    # negative => a comes first

data.sort(sortfunc=abs_cmp)
print("(cp6) custom abs sort:", data)  # expected order by |x|: -1, -2, 3, -5 or similar
print("Done")
