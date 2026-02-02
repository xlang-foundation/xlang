# 1_basic_try.x
# Basic test for try/except/finally/raise

print("CHECKPOINTS: cp1, cp2, cp3, cp4, cp5, cp6, cp7, cp8, cp9, cp10")

print("(cp1) Start Try/Except Test")

# Test 1: Simple Try/Except with Raise
print("Test 1: Catching Raise")
# Note: Syntax assumes 'except e' binds the exception to 'e'
# and 'raise expr' raises the expression value
try:
    print("Inside try 1")
    raise "Error1"
    print("(cp-error) Should not reach here after raise")
except e:
    print("(cp2) Caught exception:", e)

# Test 2: Try/Except no error
print("Test 2: No Error")
try:
    print("(cp3) Inside try no error")
except:
    print("(cp-error) Should not catch anything")

# Test 3: Try/Finally
print("Test 3: Finally")
try:
    print("Inside try for finally")
finally:
    print("(cp4) Inside finally")

# Test 4: Try/Except/Finally with error
print("Test 4: Full flow")
try:
    raise 42
    print("(cp-error) Should not reach here")
except e:
    print("(cp5) Caught:", e)
finally:
    print("(cp6) Finally executed")

# Test 5: Nested Try
print("Test 5: Nested")
try:
    try:
        raise "Inner"
    except e:
        print("(cp7) Caught Inner:", e)
    
    raise "Outer"
except e:
    print("(cp8) Caught Outer:", e)

# Test 6: Unhandled in inner, caught in outer
print("Test 6: Propagation")
try:
    try:
        raise "Propagate"
    finally:
        print("(cp9) Inner finally")
except e:
    print("(cp10) Caught propagated:", e)

print("Done")
