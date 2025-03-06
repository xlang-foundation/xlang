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

A = set("a", "e", "i", "o", "u")
#A = set(("a", "e", "i", "o", "u"))
#A = set()
print(A)
print("type of A =",type(A)) 
print(A.size())

#print(len(A)) 
#A.Remove("i")
A -= "i"
#A -= {"i"}
#A -= set("i")
print(A)
print(s.Size())

#A.Add ("z")
A += "z"
#A += "z"
print(A)
print(A.Size())
#print(A.Size())
#print(len(A)) 

B = {"red", "orange", "yellow", "green"}
#B = {1,2,3}
#B = {1.1,2.2,3.3}
print("type of B =",type(B)) 
print(B)
print(B.size())
#print(len(B)) 
#C = {"blue", "indigo", "violet"}
#B.update(C)
##print(B)
#print(len(B)) 


#B = {"red", "orange", "yellow", "green"}
#B.add ("blue")
B  += "blue"
print(B)
print(B.size())

#duplicate not allowed
#thisset = {"apple", "banana", "cherry", "apple"}
#print(thisset)

