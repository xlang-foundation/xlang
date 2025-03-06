#
# Copyright (C) 2024 The XLang Foundation
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# <END>

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
# you will get an error
#print("b is greater than a") 

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
if a > b: print("a is greater than b");print("second print")

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




