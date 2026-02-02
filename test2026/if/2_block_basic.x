
print("CHECKPOINTS: cp1, cp2, cp3, cp4")
x = 10
if x > 5:
    print("(cp1) In block: x is greater than 5") 
    x = x + 1
    print("(cp2) Incremeted x:", x) 

if x < 5:
    print("Should not be here")
else:
    print("(cp3) In else: x is not less than 5") 

print("(cp4) Done") 
