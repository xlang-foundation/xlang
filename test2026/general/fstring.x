x = 10
y = 20

print("CHECKPOINTS: cp1, cp2, cp3, cp4, cp5, cp6, cp7, cp8, cp9, cp10")

# 1. Basic
print(f"(cp1) Basic: {x}")
print("(cp1-check) Basic: 10")

# 2. Expression
print(f"(cp2) Expr: {x+1}")
print("(cp2-check) Expr: 11")

# 3. Calculation
print(f"(cp3) Calc: {x} + {y} = {x+y}")
print("(cp3-check) Calc: 10 + 20 = 30")

# 4. String variable
s = "Hello"
print(f"(cp4) {s} World")
print("(cp4-check) Hello World")

# 5. Escaped Brace
print(f"(cp5) Escaped Brace: {{ }}")
print("(cp5-check) Escaped Brace: { }")

# 6. Escaped Brace with value
print(f"(cp6) Escaped Brace with val: {{ {x} }}")
print("(cp6-check) Escaped Brace with val: { 10 }")

# 7. Dollar Expression (Legacy style inside f-string)
print(f"(cp7) Dollar Exp: ${x*2}")
print("(cp7-check) Dollar Exp: 20")

# 8. Mixed
print(f"(cp8) Mixed: {x} and ${y}")
print("(cp8-check) Mixed: 10 and 20")

# 9. Legacy (no f)
print("(cp9) Legacy: ${x*2}")
print("(cp9-check) Legacy: 20")

# 10. Legacy Const
print("(cp10) Legacy Const: ${100}")
print("(cp10-check) Legacy Const: 100")
