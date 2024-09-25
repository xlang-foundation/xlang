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

#Dictionaries are used to store data values in key:value pairs.

#Dictionary items are ordered, changeable, and does not allow duplicates.

thisdict =	{
  "brand": "Ford",
  "model": "Mustang",
  "year": 1964
}
print(thisdict)

#Print the "brand" value of the dictionary:
thisdict =	{
  "brand": "Ford",
  "model": "Mustang",
  "year": 1964
}
print(thisdict["brand"])

#When we say that dictionaries are ordered, it means that the items have a defined order, and that order will not change.
#Unordered means that the items does not have a defined order, you cannot refer to an item by using an index.

#Duplicate values will overwrite existing values:
thisdict =	{
  "brand": "Ford",
  "model": "Mustang",
  "year": 1964,
  "year": 2020
}
print(thisdict) 

#Dictionary Length
print(len(thisdict))

#dictionaries are defined as objects with the data type 'dict':
thisdict =	{
  "brand": "Ford",
  "model": "Mustang",
  "year": 1964
}
print(type(thisdict)) 

#The dict() Constructor
thisdict = dict(name = "John", age = 36, country = "Norway")
print(thisdict)
