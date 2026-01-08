
# 5_complex_flow.x
# Comprehensive test for if/elif/else, nesting, and mixed blocks

print("CHECKPOINTS: cp1, cp2, cp3, cp4, cp5, cp6, cp7, cp8, cp9, cp10")
print("(cp1) Starting Complex Flow Test") 

# Test 1: Mixed Inline and Block
a = 100
if a > 0: print("(cp2) Inline if executed") 
if a > 50:
    print("(cp3) Block if executed") 
    if a == 100: print("(cp4) Nested inline if executed") 

# Test 2: If-Elif-Else with calculations
score = 85
grade = ""
if score >= 90:
    grade = "A"
elif score >= 80:
    grade = "B"
    # Nested check inside elif
    if score >= 85:
        print("(cp5) High B") 
    else:
        print("Low B")
elif score >= 70:
    grade = "C"
else:
    grade = "F"

print("Grade:", grade) 

# Test 3: Deep Nesting with loops (using while as we have it)
# Assuming while is supported, if not, skip. But 'while' is a standard block.
count = 0
while count < 3:
    print("Loop iteration:", count) 
    if count == 0:
        print("(cp6) First")
    elif count == 1:
        print("(cp7) Second")
    else:
        print("(cp8) Third")
    count = count + 1

# Test 4: Inline Block with multiple statements (not supported in Python/XLang usually on one line with ;)
# But verifying standard inline behavior
if True: print("(cp9) Inline True 1") 

# Test 5: False conditions
if False:
    print("Error: False block executed")
else:
    print("(cp10) True: Else block executed for False condition") 

if False: print("Error: Inline False executed")

print("Done") 
