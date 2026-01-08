
print("CHECKPOINTS: cp1, cp2, cp3, cp4, cp5, cp6, cp7, cp8")

# Arithmetic
a = 10
b = 3
print("(cp1) +:", a + b) # 13
print("(cp2) -:", a - b) # 7
print("(cp3) *:", a * b) # 30
# Note: integer division might be // or / depending on implementation. Assuming python-like.
# xlang seems to support / and //.
print("(cp4) /:", a / b) # 3.333... or 3 depending on implementation

# Logic
t = True
f = False
if t and t: print("(cp5) True and True") 
if t or f: print("(cp6) True or False") 
if not f: print("(cp7) not False") 

# Precedence
x = 2 + 3 * 4
print("(cp8) Precedence 2+3*4:", x) # 14
