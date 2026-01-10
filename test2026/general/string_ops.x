# Comprehensive String Operations Test
# Tests all new Python-compatible string methods

print("CHECKPOINTS: cp1, cp2, cp3, cp4, cp5, cp6, cp7, cp8, cp9, cp10, cp11, cp12, cp13, cp14, cp15, cp16, cp17, cp18, cp19, cp20, cp21, cp22, cp23")

# Test 1: startswith
txt = "Hello, welcome to my world."
result = txt.startswith("Hello")
print(f"(cp1) startswith: {result}")
print("(cp1-check) startswith: True")

# Test 2: endswith
result = txt.endswith("world.")
print(f"(cp2) endswith: {result}")
print("(cp2-check) endswith: True")

# Test 3: isalpha
txt = "HelloWorld"
result = txt.isalpha()
print(f"(cp3) isalpha: {result}")
print("(cp3-check) isalpha: True")

# Test 4: isdigit
txt = "12345"
result = txt.isdigit()
print(f"(cp4) isdigit: {result}")
print("(cp4-check) isdigit: True")

# Test 5: isalnum
txt = "Hello123"
result = txt.isalnum()
print(f"(cp5) isalnum: {result}")
print("(cp5-check) isalnum: True")

# Test 6: isspace
txt = "   "
result = txt.isspace()
print(f"(cp6) isspace: {result}")
print("(cp6-check) isspace: True")

# Test 7: isupper
txt = "HELLO WORLD"
result = txt.isupper()
print(f"(cp7) isupper: {result}")
print("(cp7-check) isupper: True")

# Test 8: islower
txt = "hello world"
result = txt.islower()
print(f"(cp8) islower: {result}")
print("(cp8-check) islower: True")

# Test 9: capitalize
txt = "hello world"
result = txt.capitalize()
print(f"(cp9) capitalize: {result}")
print("(cp9-check) capitalize: Hello world")

# Test 10: title
txt = "hello world"
result = txt.title()
print(f"(cp10) title: {result}")
print("(cp10-check) title: Hello World")

# Test 11: swapcase
txt = "Hello World"
result = txt.swapcase()
print(f"(cp11) swapcase: {result}")
print("(cp11-check) swapcase: hELLO wORLD")

# Test 12: lstrip
txt = "   banana   "
result = txt.lstrip()
print(f"(cp12) lstrip: {result}")
print("(cp12-check) lstrip: banana   ")

# Test 13: rstrip
txt = "   banana   "
result = txt.rstrip()
print(f"(cp13) rstrip: {result}")
print("(cp13-check) rstrip:    banana")

# Test 14: center
txt = "banana"
result = txt.center(20)
print(f"(cp14) center: {result}")
print("(cp14-check) center:        banana       ")

# Test 15: ljust
txt = "banana"
result = txt.ljust(20)
print(f"(cp15) ljust: {result}")
print("(cp15-check) ljust: banana              ")

# Test 16: rjust
txt = "banana"
result = txt.rjust(20)
print(f"(cp16) rjust: {result}")
print("(cp16-check) rjust:               banana")

# Test 17: zfill
txt = "50"
result = txt.zfill(10)
print(f"(cp17) zfill: {result}")
print("(cp17-check) zfill: 0000000050")

# Test 18: count
txt = "I love apples, apple are my favorite fruit"
result = txt.count("apple")
print(f"(cp18) count: {result}")
print("(cp18-check) count: 2")

# Test 19: index
txt = "Hello, welcome to my world."
result = txt.index("welcome")
print(f"(cp19) index: {result}")
print("(cp19-check) index: 7")

# Test 20: split with index [0]
txt = "apple;banana;cherry"
parts = txt.split(";")
result = parts[0]
print(f"(cp20) split[0]: {result}")
print("(cp20-check) split[0]: apple")

# Test 21: split with index [-1]
txt = "apple;banana;cherry"
parts = txt.split(";")
result = parts[-1]
print(f"(cp21) split[-1]: {result}")
print("(cp21-check) split[-1]: cherry")

# Test 22: split with index [1]
txt = "path/to/my/file.txt"
parts = txt.split("/")
result = parts[1]
print(f"(cp22) split[1]: {result}")
print("(cp22-check) split[1]: to")

# Test 23: Complex operations chain
txt = "  HELLO WORLD  "
result = txt.strip().capitalize()
print(f"(cp23) chain: {result}")
print("(cp23-check) chain: Hello world")
