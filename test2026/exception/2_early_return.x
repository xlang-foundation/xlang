# 2_early_return.x
# Test return behavior in try/finally

def test_ret():
    try:
        print("Inside try")
        return "ReturnedValue"
    finally:
        print("Inside finally")

print("Start") # EXPECT: Start
ret = test_ret()
print("Result:", ret) # EXPECT: Result: ReturnedValue
print("Done") # EXPECT: Done
