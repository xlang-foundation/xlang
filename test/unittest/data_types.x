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

"""
following data types built-in by default, in these categories:

Text Type:	str
Numeric Types:	int, float, complex
Sequence Types:	list, tuple, range
Mapping Type:	dict
Set Types:	set, frozenset
Boolean Type:	bool
Binary Types:	bytes, bytearray, memoryview
None Type:	NoneType
"""

#Getting the Data Type
x = 5
print(type(x))

#Setting the Data Type

x = "Hello World"			#str	
print(x)

x = 20					#int	
print(x)

x = 20.5				#float	
print(x)

x = 1j					#complex	
print(x)

x = ["apple", "banana", "cherry"]	#list	
print(x)

x = ("apple", "banana", "cherry")	#tuple	
print(x)

x = range(6)				#range	
print(x)

x = {"name" : "John", "age" : 36}	#dict	
print(x)

x = {"apple", "banana", "cherry"}	#set	
print(x)

x = frozenset({"apple", "banana", "cherry"})	#frozenset	
print(x)

x = True				#bool	
print(x)

x = b"Hello"				#bytes	
print(x)

x = bytearray(5)			#bytearray	
print(x)

x = memoryview(bytes(5))		#memoryview	
print(x)

x = None				#NoneType
print(x)


#Setting the Specific Data Type
x = str("Hello World")			#str	
print(x)

x = int(20)				#int	
print(x)

x = float(20.5)				#float	
print(x)

x = complex(1j)				#complex	
print(x)

x = list(("apple", "banana", "cherry"))	#list	
print(x)

x = tuple(("apple", "banana", "cherry")) #tuple	
print(x)

x = range(6)				#range	
print(x)

x = dict(name="John", age=36)		#dict	print(x)
print(x)

x = set(("apple", "banana", "cherry"))	#set	
print(x)

x = frozenset(("apple", "banana", "cherry"))	#frozenset	
print(x)

x = bool(5)				#bool	
print(x)

x = bytes(5)				#bytes	
print(x)

x = bytearray(5)			#bytearray	
print(x)

x = memoryview(bytes(5))		#memoryview
print(x)


