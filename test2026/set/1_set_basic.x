
print("CHECKPOINTS: cp1, cp2, cp3, cp4, cp5, cp6, cp7, cp8")

# --- cp1: Basic set creation and print ---
s = set(1, 2, 3, 2, 1)
print("(cp1) set:", s)       # EXPECT: {1, 2, 3}

# --- cp2: add ---
s.add(4)
s.add(2)   # duplicate, no-op
print("(cp2) after add:", s.size())  # EXPECT size is 4

# --- cp3: discard (no-op if missing) ---
s.discard(99)   # should not raise
s.discard(4)
print("(cp3) after discard:", s.size())  # 3 elements remain

# --- cp4: remove (returns bool) ---
ok = s.remove(3)
print("(cp4) remove ok:", ok)   # EXPECT True

# --- cp5: contains (in operator) ---
result = 1 in s
print("(cp5) 1 in s:", result)  # EXPECT True

# --- cp6: pop ---
prev_size = s.size()
v = s.pop()
print("(cp6) popped, new size:", s.size())   # one less

# --- cp7: copy ---
a = set(10, 20, 30)
b = a.copy()
b.add(40)
print("(cp7) a.size:", a.size(), "b.size:", b.size())  # a=3, b=4

# --- cp8: clear ---
a.clear()
print("(cp8) after clear:", a.size())  # EXPECT 0
