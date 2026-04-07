
def func1(x,y):
    return x+y

c = func1.getcode()
print("Code:", c)

ret = func1.call(10,20)
print("Call(10,20) Result:", ret)

# Test with list args
ret2 = func1.call([30,40])
print("Call([30,40]) Result:", ret2)

# Test with kwargs? (func1 doesn't use kwargs but let's see if call accepts it)
# func1.call([50,60], {"extra":1}) 
