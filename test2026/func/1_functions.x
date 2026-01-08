
print("CHECKPOINTS: cp1, cp2, cp3, cp4, cp5, cp6")

def add(a, b):
    return a + b

res = add(10, 20)
print("(cp1) add(10,20):", res) 

# Recursion
def fact(n):
    if n <= 1:
        return 1
    return n * fact(n - 1)

f = fact(5)
print("(cp2) fact(5):", f) 

# Default params (if supported) check output behavior
def greet(name, msg="Hello"):
    print("(cp3)", msg, name) 

# Assuming defaults supported syntax-wise
greet("World") 
greet("Xlang", "Hi") 

# Closure / Nested function
def outer(x):
    def inner(y):
        return x + y
    return inner

fn = outer(100)
res2 = fn(50)
print("(cp4) closure result:", res2) 

print("(cp5) Done") 
# Add extra cp just to be safe
print("(cp6) Final") 
