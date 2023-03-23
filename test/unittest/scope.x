
#A variable created outside of a function is global and can be used by anyone

#Local Scope
def myfunc():
  x = 100
  print(x)

myfunc()

#Function Inside Function
def myfunc():
  x = 200
  def myinnerfunc():
    print(x)
  myinnerfunc()

myfunc()

#Global Scope
x = 300

def myfunc():
  print(x)

myfunc()

print(x)

#If you operate with the same variable name inside and outside of a function, Python will treat them as two separate variables
x = 300

def myfunc():
  x = 200
  print(x)

myfunc()

print(x)

#Global Keyword
def myfunc():
  global x
  x = 300

myfunc()

print(x)

#Also, use the global keyword if you want to make a change to a global variable inside a function.
x = 300

def myfunc():
  global x
  x = 800

myfunc()

print(x)
