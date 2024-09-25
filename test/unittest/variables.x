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

#Creating Variables
x = 5
y = "John"
print(x)
print(y)

# change type 
x = 4       # x is of type int
print(x)
x = "Sally" # x is now of type str
print(x)

#Casting
x = str(3)    # x will be '3'
y = int(3)    # y will be 3
z = float(3)  # z will be 3.0
print(x)
print(y)
print(z)

#Get the Type
x = 5
y = "John"
print(type(x))
print(type(y))
z=type(x)
print(z)
print(type(z))

#Single or Double Quotes?
x = "John"
print(x)
# is the same as
x = 'John'
print(x)

#Case-Sensitive
a = 4
A = "Sally"
#A will not overwrite a
print(a)
print(A)

