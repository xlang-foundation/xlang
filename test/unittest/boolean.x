#Booleans

print(10 > 9)
print(10 == 9)
print(10 < 9) 

a = 200
b = 33

#run a condition in an if statement, Python returns True or False
if b > a:
  print("b is greater than a")
else:
  print("b is not greater than a") 

#Evaluate Values and Variables with bool() function
print(bool("Hello"))
print(bool(15))

x = "Hello"
y = 15

print(bool(x))
print(bool(y))

"""
Most Values are True

Almost any value is evaluated to True if it has some sort of content.

Any string is True, except empty strings.

Any number is True, except 0.

Any list, tuple, set, and dictionary are True, except empty ones.
"""
bool("abc")
bool(123)
bool(["apple", "cherry", "banana"])

#Some Values are False
#empty values, such as (), [], {}, "", the number 0, and the value None. And of course the value False evaluates to False.
bool(False)
bool(None)
bool(0)
bool("")
bool(())
bool([])
bool({}) 

#an object that is made from a class with a __len__ function that returns 0 or False
class myclass():
  def __len__(self):
    return 0

myobj = myclass()
print(bool(myobj))

#Functions can Return a Boolean
def myFunction() :
  return True

print(myFunction()) 

#built-in functions 
x = 200
print(isinstance(x, int)) 





