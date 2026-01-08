# Advanced F-String Tests

print("CHECKPOINTS: cp1, cp2, cp3, cp4, cp5, cp6, cp7, cp8, cp9, cp10")
print("Start Advanced Tests")

# 1. Whitespace handling in expressions
val = 100
print(f"(cp1) WS: {  val  }")
print("(cp1-check) WS: 100")

# 2. Arithmetic and logic
print(f"(cp2) Calc: { val * 2 + 50 }")
print("(cp2-check) Calc: 250")

# 3. List access
l = [10, 20, 30]
print(f"(cp3) List: {l[1]}")
print("(cp3-check) List: 20")

# 4. Dict access (string keys needing quotes)
d = {'k': "v"}
print(f"(cp4) Dict: {d['k']}")
print("(cp4-check) Dict: v")

# 5. Function calls
def my_sq(x):
    return x*x

print(f"(cp5) Func: {my_sq(5)}")
print("(cp5-check) Func: 25")

# 6. Nested Calls/Expressions
print(f"(cp6) Nested: {my_sq(l[0]) + 1}")
print("(cp6-check) Nested: 101")

# 7. Multi-expression f-string
a = 1
b = 2
print(f"(cp7) Multi: {a} -- {b} -- {a+b}")
print("(cp7-check) Multi: 1 -- 2 -- 3")

# 8. Escaping check again
print(f"(cp8) Braces: {{ and }} with {a}")
print("(cp8-check) Braces: { and } with 1")

# 9. Mixed legacy and f-string
print(f"(cp9) Legacy in F: ${a+b}")
print("(cp9-check) Legacy in F: 3")

# 10. String with special chars
print(f"(cp10) Special: Tab\tNewline\nEnd")
print("(cp10-check) Special: Tab	Newline")
# (cp10-check) End (Note: checking multiline manually or via simple contains)

print("End Advanced Tests")
