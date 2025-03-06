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
