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


