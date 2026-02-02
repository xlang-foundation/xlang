
print("CHECKPOINTS: cp1, cp2, cp3, cp4")
x = 10
y = 20
if x == 10:
    print("(cp1) x is 10") 
    if y == 20:
        print("(cp2) y is 20") 
        if x < y: print("(cp3) x < y inline nested") 
    else:
        print("y is not 20")
else:
    print("x is not 10")
print("(cp4) Done") 
