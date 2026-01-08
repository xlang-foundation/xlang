
print("CHECKPOINTS: cp1, cp2, cp3, cp4")

# Basic List
l = [1, 2, 3]
print("(cp1) list:", l) 
print("(cp2) len:", len(l)) # Assuming len() builtin

l.append(4)
print("(cp3) after append:", l) 

# Indexing
print("(cp4) l[0]:", l[0]) 

# Note: Slicing not always implemented effectively in early parsers, testing basic access first.
