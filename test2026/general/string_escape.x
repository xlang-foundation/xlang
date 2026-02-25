# String Escape Sequence Tests
# Verifies that \n, \t, \\, \xHH are decoded to real characters

print("CHECKPOINTS: cp1, cp2, cp3, cp4, cp5, cp6")

# cp1: \n produces real newline
s = "line1\nline2"
parts = s.split("\n")
result = parts[0]
print(f"(cp1) newline-split: {result}")
print("(cp1-check) newline-split: line1")

# cp2: \t produces real tab - round-trip split on tab
s = "alpha\tbeta"
parts = s.split("\t")
result = parts[1]
print(f"(cp2) tab-split: {result}")
print("(cp2-check) tab-split: beta")

# cp3: \\ produces single backslash
s = "before\\after"
parts = s.split("\\")
result = parts[0]
print(f"(cp3) backslash-split: {result}")
print("(cp3-check) backslash-split: before")

# cp4: \n inside f-string also works
name = "World"
s = f"Hello\n{name}"
parts = s.split("\n")
result = parts[1]
print(f"(cp4) fstring-newline: {result}")
print("(cp4-check) fstring-newline: World")

# cp5: hex escape \x41 = 'A'
s = "\x41\x42\x43"
print(f"(cp5) hex-escape: {s}")
print("(cp5-check) hex-escape: ABC")

# cp6: multiple escape types combined
s = "a\tb\nc"
parts = s.split("\n")
first_part = parts[0]
tabs = first_part.split("\t")
result = tabs[1]
print(f"(cp6) combined-escape: {result}")
print("(cp6-check) combined-escape: b")
