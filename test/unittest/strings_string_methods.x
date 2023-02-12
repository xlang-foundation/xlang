#String Methods

#a set of built-in methods that you can use on strings

#capitalize()  - Converts the first character to upper case
txt = "hello, and welcome to my world."
x = txt.capitalize()
print (x)

#The first character is converted to upper case, and the rest are converted to lower case
txt = "python is FUN!"
x = txt.capitalize()
print (x) 

#See what happens if the first character is a number
txt = "36 is my age."
x = txt.capitalize()
print (x) 

#casefold()	Converts string into lower case
txt = "Hello, And Welcome To My World!"
x = txt.casefold()
print(x) 

#center()	Returns a centered string
#Print the word "banana", taking up the space of 20 characters, with "banana" in the middle:
txt = "banana"
x = txt.center(20)
print(x) 

#Using the letter "O" as the padding character
txt = "banana"
x = txt.center(20, "O")
print(x) 


#count()	Returns the number of times a specified value occurs in a string
txt = "I love apples, apple are my favorite fruit"
x = txt.count("apple")
print(x) 

#Search from position 10 to 24
txt = "I love apples, apple are my favorite fruit"
x = txt.count("apple", 10, 24)
print(x)

#encode()	Returns an encoded version of the string
#UTF-8 encode the string
txt = "My name is Ståle"
x = txt.encode()
print(x) 

# uses ascii encoding, and a character that cannot be encoded, showing the result with different errors
txt = "My name is Ståle"
print(txt.encode(encoding="ascii",errors="backslashreplace"))
print(txt.encode(encoding="ascii",errors="ignore"))
print(txt.encode(encoding="ascii",errors="namereplace"))
print(txt.encode(encoding="ascii",errors="replace"))
print(txt.encode(encoding="ascii",errors="xmlcharrefreplace"))


#endswith()	Returns true if the string ends with the specified value
#Check if the string ends with a punctuation sign (.)
txt = "Hello, welcome to my world."
x = txt.endswith(".")
print(x) 

#Check if the string ends with the phrase "my world."
txt = "Hello, welcome to my world."
x = txt.endswith("my world.")
print(x) 

#Check if position 5 to 11 ends with the phrase "my world.":
txt = "Hello, welcome to my world."
x = txt.endswith("my world.", 5, 11)
print(x) 

#expandtabs()	Sets the tab size of the string

txt = "H\te\tl\tl\to"
x =  txt.expandtabs(2)
print(x)

txt = "H\te\tl\tl\to"
print(txt)
print(txt.expandtabs())
print(txt.expandtabs(2))
print(txt.expandtabs(4))
print(txt.expandtabs(10)) 

#find()	Searches the string for a specified value and returns the position of where it was found
#string.find(value, start, end) 

#first occurrence of a string
txt = "Hello, welcome to my world."
x = txt.find("welcome")
print(x) 

#first occurrence of a letter
txt = "Hello, welcome to my world."
x = txt.find("e")
print(x) 

#first occurrence of the letter "e" when you only search between position 5 and 10
txt = "Hello, welcome to my world."
x = txt.find("e", 5, 10)
print(x) 

#If the value is not found, the find() method returns -1, but the index() method will raise an exception
txt = "Hello, welcome to my world."
print(txt.find("q"))
print(txt.index("q")) 

#==================================================================================
#format()	Formats specified values in a string

#Insert the price inside the placeholder, the price should be in fixed point, two-decimal format
txt = "For only {price:.2f} dollars!"
print(txt.format(price = 49)) 

#Using different placeholder values
txt1 = "My name is {fname}, I'm {age}".format(fname = "John", age = 36)
txt2 = "My name is {0}, I'm {1}".format("John",36)
txt3 = "My name is {}, I'm {}".format("John",36) 
print(txt1)
print(txt2)
print(txt3)

#To demonstrate, we insert the number 8 to set the available space for the value to 8 characters.

#Use "<" to left-align the value:
txt = "We have {:<8} chickens."
print(txt.format(49))

#Use ">" to right-align the value:
txt = "We have {:>8} chickens."
print(txt.format(49))

#Use "^" to center-align the value:
txt = "We have {:^8} chickens."
print(txt.format(49))

#Use "=" to place the plus/minus sign at the left most position:
txt = "The temperature is {:=8} degrees celsius."
print(txt.format(-5))

#Use "+" to always indicate if the number is positive or negative:
txt = "The temperature is between {:+} and {:+} degrees celsius."
print(txt.format(-3, 7))

#Use "-" to always indicate if the number is negative (positive numbers are displayed without any sign):
txt = "The temperature is between {:-} and {:-} degrees celsius."
print(txt.format(-3, 7))

#Use " " (a space) to insert a space before positive numbers and a minus sign before negative numbers:
txt = "The temperature is between {: } and {: } degrees celsius."
print(txt.format(-3, 7))

#Use "," to add a comma as a thousand separator:
txt = "The universe is {:,} years old."
print(txt.format(13800000000))

#Use "_" to add a underscore character as a thousand separator:
txt = "The universe is {:_} years old."
print(txt.format(13800000000))

#Use "b" to convert the number into binary format:
txt = "The binary version of {0} is {0:b}"
print(txt.format(5))

#Use "d" to convert a number, in this case a binary number, into decimal number format:
txt = "We have {:d} chickens."
print(txt.format(0b101))

#Use "e" to convert a number into scientific number format (with a lower-case e):
txt = "We have {:e} chickens."
print(txt.format(5))

#Use "E" to convert a number into scientific number format (with an upper-case E):
txt = "We have {:E} chickens."
print(txt.format(5))

#Use "f" to convert a number into a fixed point number, default with 6 decimals, but use a period followed by a number to specify the number of decimals:
txt = "The price is {:.2f} dollars."
print(txt.format(45))
#without the ".2" inside the placeholder, this number will be displayed like this:
txt = "The price is {:f} dollars."
print(txt.format(45))

#Use "F" to convert a number into a fixed point number, but display inf and nan as INF and NAN:
x = float('inf')
txt = "The price is {:F} dollars."
print(txt.format(x))
#same example, but with a lower case f:
txt = "The price is {:f} dollars."
print(txt.format(x))

#Use "o" to convert the number into octal format:
txt = "The octal version of {0} is {0:o}"
print(txt.format(10))

#Use "x" to convert the number into Hex format:
txt = "The Hexadecimal version of {0} is {0:x}"
print(txt.format(255))

#Use "X" to convert the number into upper-case Hex format:
txt = "The Hexadecimal version of {0} is {0:X}"
print(txt.format(255))

#Use "%" to convert the number into a percentage format:
txt = "You scored {:%}"
print(txt.format(0.25))
#Or, without any decimals:
txt = "You scored {:.0%}"
print(txt.format(0.25))

#==================================================================================

#format_map()	Formats specified values in a string
#index()	Searches the string for a specified value and returns the position of where it was found
#isalnum()	Returns True if all characters in the string are alphanumeric
#isalpha()	Returns True if all characters in the string are in the alphabet
#isdecimal()	Returns True if all characters in the string are decimals
#isdigit()	Returns True if all characters in the string are digits
#isidentifier()	Returns True if the string is an identifier
#islower()	Returns True if all characters in the string are lower case
#isnumeric()	Returns True if all characters in the string are numeric
#isprintable()	Returns True if all characters in the string are printable
#isspace()	Returns True if all characters in the string are whitespaces
#istitle() 	Returns True if the string follows the rules of a title
#isupper()	Returns True if all characters in the string are upper case
#join()	Joins the elements of an iterable to the end of the string
#ljust()	Returns a left justified version of the string
#lower()	Converts a string into lower case
#lstrip()	Returns a left trim version of the string
#maketrans()	Returns a translation table to be used in translations
#partition()	Returns a tuple where the string is parted into three parts
#replace()	Returns a string where a specified value is replaced with a specified value
#rfind()	Searches the string for a specified value and returns the last position of where it was found
#rindex()	Searches the string for a specified value and returns the last position of where it was found
#rjust()	Returns a right justified version of the string
#rpartition()	Returns a tuple where the string is parted into three parts
#rsplit()	Splits the string at the specified separator, and returns a list
#rstrip()	Returns a right trim version of the string
#split()	Splits the string at the specified separator, and returns a list
#splitlines()	Splits the string at line breaks and returns a list
#startswith()	Returns true if the string starts with the specified value
#strip()	Returns a trimmed version of the string
#swapcase()	Swaps cases, lower case becomes upper case and vice versa
#title()	Converts the first character of each word to upper case
#translate()	Returns a translated string
#upper()	Converts a string into upper case
#zfill()	Fills the string with a specified number of 0 values at the beginning


