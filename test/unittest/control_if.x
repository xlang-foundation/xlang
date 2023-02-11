
#Conditions and If statements

"""
Equals: a == b
Not Equals: a != b
Less than: a < b
Less than or equal to: a <= b
Greater than: a > b
Greater than or equal to: a >= b
"""

a = 33
b = 200
if b > a:
  print("b is greater than a")

#Indentation
#a = 33
#b = 200
#if b > a:
#print("b is greater than a") # you will get an error

#The elif keyword 
a = 33
b = 33
if b > a:
  print("b is greater than a")
elif a == b:
  print("a and b are equal")

#The else keyword
a = 200
b = 33
if b > a:
  print("b is greater than a")
elif a == b:
  print("a and b are equal")
else:
  print("a is greater than b")


#Short Hand If
a = 200
b = 33
if a > b: print("a is greater than b")

#Short Hand If ... Else
a = 2
b = 330
print("A") if a > b else print("B")

# Ternary Operators, or Conditional Expressions
a = 330
b = 330
print("A") if a > b else print("=") if a == b else print("B")

#And  keyword
a = 200
b = 33
c = 500
if a > b and c > a:
  print("Both conditions are True")

#or keyword
a = 200
b = 33
c = 500
if a > b or a > c:
  print("At least one of the conditions is True")

#not keyword
a = 33
b = 200
if not a > b:
  print("a is NOT greater than b")


#Nested If
x = 41

if x > 10:
  print("Above ten,")
  if x > 20:
    print("and also above 20!")
  else:
    print("but not above 20.")

#The pass Statement
a = 33
b = 200

if b > a:
  pass
else:
  print ("a >= b")




