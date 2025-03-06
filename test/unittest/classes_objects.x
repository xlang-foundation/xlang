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

#Create a Class
class MyClass:
  x = 5

print(MyClass)

#Create an Object
p1 = MyClass()
print(p1.x)

#The __init__() Function, which is always executed when the class is being initiated
class Person:
  def __init__(name, age):
    this.name = name
    this.age = age

p1 = Person("John", 36)

print(p1.name)
print(p1.age)
