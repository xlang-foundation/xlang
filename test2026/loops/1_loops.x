
print("CHECKPOINTS: cp1, cp2, cp3, cp4, cp5, cp6, cp7, cp8, cp9")

# While loop
i = 0
while i < 3:
    print("(cp1) while i:", i) 
    # EXPECT(cp1) 0, 1, 2 (but checkpoints are just presence checks, so seeing it once is enough for the checker, 
    # but we should probably differentiate loop iterations if we want strictness.
    # For now, simplistic check.)
    i = i + 1

# Break
jj = 0
while jj < 5:
    if jj == 2:
        print("(cp2) breaking at 2") 
        break
    print("(cp3) jj:", jj) 
    jj = jj + 1

# Continue
k = 0
while k < 3:
    k = k + 1
    if k == 2:
        print("(cp4) skipping 2") 
        continue
    print("(cp5) k:", k) 
    # Should print k: 1, k: 3

# For loop (assuming xlang supports 'range' or list iteration)
# Testing basic list iteration
l = [1, 2, 3]
for x in l:
    print("(cp6) for x:", x) 

# Nested Loop
x = 0
while x < 2:
    print("(cp7) outer:", x) 
    y = 0
    while y < 2:
        print("(cp8) inner:", y) 
        y = y + 1
    x = x + 1

print("(cp9) Done") 
