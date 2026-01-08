
print("CHECKPOINTS: cp1, cp2, cp3")

class Point:
    def __init__(x, y):
        this.x = x
        this.y = y
    
    def print_me():
        print("(cp1) Point:", this.x, this.y) 

p = Point(10, 20)
p.print_me()

# Basic Method call
print("(cp2) p.x:", p.x) 

print("(cp3) Done") 
