
print("CHECKPOINTS: cp1, cp2, cp3, cp4")

a = set(1, 2, 3)
b = set(3, 4, 5)

# --- cp1: update (in-place union) ---
a.update(b)
print("(cp1) after update size:", a.size())   # 5: {1,2,3,4,5}

# --- cp2: intersection_update ---
x = set(1, 2, 3, 4)
y = set(3, 4, 5, 6)
x.intersection_update(y)
print("(cp2) after intersect_update size:", x.size())  # 2: {3,4}

# --- cp3: difference_update ---
p = set(1, 2, 3, 4, 5)
q = set(3, 4)
p.difference_update(q)
print("(cp3) after diff_update size:", p.size())   # 3: {1,2,5}

# --- cp4: symmetric_difference_update ---
m = set(1, 2, 3)
n = set(3, 4, 5)
m.symmetric_difference_update(n)
print("(cp4) after sym_diff_update size:", m.size())   # 4: {1,2,4,5}
