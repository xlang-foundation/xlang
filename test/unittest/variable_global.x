#Global Variables

x = "awesome"

def myfunc():
  print("Python is " + x)

myfunc()


# a variable with the same name inside a function
x = "awesome"

def myfunc():
  x = "fantastic"
  print("Python is " + x)

myfunc()


#The global Keyword
def myfunc():
  global x
  x = "fantastic"

myfunc()

print("Python is " + x)

#change the value of a global variable inside a function
x = "awesome"

def myfunc():
  global x
  x = "fantastic"

myfunc()

print("Python is " + x)



