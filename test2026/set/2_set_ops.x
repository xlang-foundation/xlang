
print("CHECKPOINTS: cp1, cp2, cp3, cp4")

a = set(1, 2, 3)
b = set(3, 4, 5)

# --- cp1: union ---
u = a.union(b)
print("(cp1) union size:", u.size())   # 5 elements: 1,2,3,4,5

# --- cp2: intersection ---
inter = a.intersection(b)
print("(cp2) intersection size:", inter.size())   # 1 element: 3

# --- cp3: difference ---
diff = a.difference(b)
print("(cp3) difference size:", diff.size())   # 2 elements: 1,2

# --- cp4: symmetric_difference ---
sym = a.symmetric_difference(b)
print("(cp4) sym_diff size:", sym.size())   # 4 elements: 1,2,4,5
