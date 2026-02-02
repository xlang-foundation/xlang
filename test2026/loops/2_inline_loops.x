
print("CHECKPOINTS: cp1, cp2, cp3, cp4")

# Inline While
i = 0
while i < 3: print("(cp1) inline while:", i); i = i + 1

# Inline For (assuming range or List)
l = [10, 20]
for x in l: print("(cp2) inline for:", x)

# Inline Loop with 'break' (semicolon support check)
idx = 0
while idx < 5: idx = idx + 1; if idx == 3: print("(cp3) break inline"); break

print("(cp4) Done")
