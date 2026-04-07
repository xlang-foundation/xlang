
print("CHECKPOINTS: cp1, cp2, cp3")

a = set(1, 2, 3)
b = set(1, 2, 3, 4, 5)
c = set(4, 5, 6)

# --- cp1: issubset ---
r1 = a.issubset(b)
r2 = b.issubset(a)
print("(cp1) a<=b:", r1, "b<=a:", r2)  # True False

# --- cp2: issuperset ---
r3 = b.issuperset(a)
r4 = a.issuperset(b)
print("(cp2) b>=a:", r3, "a>=b:", r4)  # True False

# --- cp3: isdisjoint ---
r5 = a.isdisjoint(c)
r6 = a.isdisjoint(b)
print("(cp3) a disjoint c:", r5, "a disjoint b:", r6)  # True False
