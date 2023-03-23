#String Methods

#a set of built-in methods that you can use on strings

#capitalize()  - Converts the first character to upper case
txt = "hello, and welcome to my world."
x = txt.capitalize()
print (x)

#The first character is converted to upper case, and the rest are converted to lower case
txt = "Xlang is FUN!"
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
txt = "Hello, welcome to my world."
x = txt.index("welcome")
print(x) 

#Where in the text is the first occurrence of the letter "e" when you only search between position 5 and 10?
txt = "Hello, welcome to my world."
x = txt.index("e", 5, 10)
print(x) 

#If the value is not found, the find() method returns -1, but the index() method will raise an exception
txt = "Hello, welcome to my world."
print(txt.find("q"))
print(txt.index("q")) 

#isalnum()	Returns True if all characters in the string are alphanumeric
txt = "Company12"
x = txt.isalnum()
print(x) 

txt = "Company 12"
x = txt.isalnum()
print(x) 

#isalpha()	Returns True if all characters in the string are in the alphabet
txt = "CompanyX"
x = txt.isalpha()
print(x) 

txt = "Company10"
x = txt.isalpha()
print(x) 

#isdecimal()	Returns True if all characters in the string are decimals
txt = "\u0033" #unicode for 3
x = txt.isdecimal()
print(x) 

#Check if all the characters in the unicode are decimals
a = "\u0030" #unicode for 0
b = "\u0047" #unicode for G
print(a.isdecimal())
print(b.isdecimal()) 

#isdigit()	Returns True if all characters in the string are digits
txt = "50800"
x = txt.isdigit()
print(x) 

#Check if all the characters in the text are digits
a = "\u0030" #unicode for 0
b = "\u00B2" #unicode for ²
print(a.isdigit())
print(b.isdigit()

#isidentifier()	Returns True if the string is an identifier

#Check if the string is a valid identifier
txt = "Demo"
x = txt.isidentifier()
print(x) 

#Check if the strings are valid identifiers:
a = "MyFolder"
b = "Demo002"
c = "2bring"
d = "my demo"

print(a.isidentifier())
print(b.isidentifier())
print(c.isidentifier())
print(d.isidentifier())


#islower()	Returns True if all characters in the string are lower case
txt = "hello world!"
x = txt.islower()
print(x) 

#Check if all the characters in the texts are in lower case
a = "Hello world!"
b = "hello 123"
c = "mynameisPeter"

print(a.islower())
print(b.islower())
print(c.islower()) 

#isnumeric()	Returns True if all characters in the string are numeric
txt = "565543"
x = txt.isnumeric()
print(x) 

a = "\u0030" #unicode for 0
b = "\u00B2" #unicode for &sup2;
c = "10km2"
d = "-1"
e = "1.5"

print(a.isnumeric())
print(b.isnumeric())
print(c.isnumeric())
print(d.isnumeric())
print(e.isnumeric()) 

#isprintable()	Returns True if all characters in the string are printable
txt = "Hello! Are you #1?"
x = txt.isprintable()
print(x) 

txt = "Hello!\nAre you #1?"
x = txt.isprintable()
print(x) 

#isspace()	Returns True if all characters in the string are whitespaces
txt = "   "
x = txt.isspace()
print(x) 

txt = "   s   "
x = txt.isspace()
print(x) 

#istitle() 	Returns True if the string follows the rules of a title
#Check if each word start with an upper case letter:
txt = "Hello, And Welcome To My World!"
x = txt.istitle()
print(x) 

a = "HELLO, AND WELCOME TO MY WORLD"
b = "Hello"
c = "22 Names"
d = "This Is %'!?"

print(a.istitle())
print(b.istitle())
print(c.istitle())
print(d.istitle())

#isupper()	Returns True if all characters in the string are upper case
txt = "THIS IS NOW!"
x = txt.isupper()
print(x)

a = "Hello World!"
b = "hello 123"
c = "MY NAME IS PETER"
print(a.isupper())
print(b.isupper())
print(c.isupper()) 

#join()	Joins the elements of an iterable to the end of the string
#Join all items in a tuple into a string, using a hash character as separator:
myTuple = ("John", "Peter", "Vicky")
x = "#".join(myTuple)
print(x) 

#Join all items in a dictionary into a string, using the word "TEST" as separator:
myDict = {"name": "John", "country": "Norway"}
mySeparator = "TEST"
x = mySeparator.join(myDict)
print(x)

#ljust()	Returns a left justified version of the string
#Return a 20 characters long, left justified version of the word "banana"
txt = "banana"
x = txt.ljust(20)
print(x, "is my favorite fruit.")

#Using the letter "O" as the padding character:
txt = "banana"
x = txt.ljust(20, "O")
print(x) 

#lower()	Converts a string into lower case
txt = "Hello my FRIENDS"
x = txt.lower()
print(x)


#lstrip()	Returns a left trim version of the string
txt = "     banana     "
x = txt.lstrip()
print("of all fruits", x, "is my favorite") 

txt = ",,,,,ssaaww.....banana"
x = txt.lstrip(",.asw")
print(x) 

#maketrans()	Returns a translation table to be used in translations
#Create a mapping table, and use it in the translate() method to replace any "S" characters with a "P" character
txt = "Hello Sam!"
mytable = str.maketrans("S", "P")
print(txt.translate(mytable))

#Use a mapping table to replace many characters
txt = "Hi Sam!"
x = "mSa"
y = "eJo"
mytable = str.maketrans(x, y)
print(txt.translate(mytable)) 

#The third parameter in the mapping table describes characters that you want to remove from the string
txt = "Good night Sam!"
x = "mSa"
y = "eJo"
z = "odnght"
mytable = str.maketrans(x, y, z)
print(txt.translate(mytable)) 

#The maketrans() method itself returns a dictionary describing each replacement, in unicode
txt = "Good night Sam!"
x = "mSa"
y = "eJo"
z = "odnght"
print(str.maketrans(x, y, z)) 

#partition()	Returns a tuple where the string is parted into three parts
#Search for the word "bananas", and return a tuple with three elements:
txt = "I could eat bananas all day"
x = txt.partition("bananas")
print(x) 

#If the specified value is not found, the partition() method returns a tuple containing: 1 - the whole string, 2 - an empty string, 3 - an empty string
txt = "I could eat bananas all day"
x = txt.partition("apples")
print(x) 

#replace()	Returns a string where a specified value is replaced with a specified value
#Replace all occurrence of the word "one":
txt = "one one was a race horse, two two was one too."
x = txt.replace("one", "three")
print(x) 

#Replace the two first occurrence of the word "one"
txt = "one one was a race horse, two two was one too."
x = txt.replace("one", "three", 2)
print(x) 

#rfind()	Searches the string for a specified value and returns the last position of where it was found
#string
txt = "Mi casa, su casa."
x = txt.rfind("casa")
print(x) 

#letter
txt = "Hello, welcome to my world."
x = txt.rfind("e")
print(x)

#Where in the text is the last occurrence of the letter "e" when you only search between position 5 and 10
txt = "Hello, welcome to my world."
x = txt.rfind("e", 5, 10)
print(x) 

#If the value is not found, the rfind() method returns -1, but the rindex() method will raise an exception
txt = "Hello, welcome to my world."
print(txt.rfind("q"))
print(txt.rindex("q")) 


#rindex()	Searches the string for a specified value and returns the last position of where it was found
#last occurrence of a string
txt = "Mi casa, su casa."
x = txt.rindex("casa")
print(x) 

#Where in the text is the last occurrence of the letter "e"?:
txt = "Hello, welcome to my world."
x = txt.rindex("e")
print(x) 

#Where in the text is the last occurrence of the letter "e" when you only search between position 5 and 10?
txt = "Hello, welcome to my world."
x = txt.rindex("e", 5, 10)
print(x) 

#If the value is not found, the rfind() method returns -1, but the rindex() method will raise an exception
txt = "Hello, welcome to my world."
print(txt.rfind("q"))
print(txt.rindex("q"))


#rjust()	Returns a right justified version of the string
#Return a 20 characters long, right justified version of the word "banana"
txt = "banana"
x = txt.rjust(20)
print(x, "is my favorite fruit.") 

#Using the letter "O" as the padding character
txt = "banana"
x = txt.rjust(20, "O")
print(x)


#rpartition()	Returns a tuple where the string is parted into three parts
#Search for the last occurrence of the word "bananas", and return a tuple with three elements
txt = "I could eat bananas all day, bananas are my favorite fruit"
x = txt.rpartition("bananas")
print(x) 

#If the specified value is not found, the rpartition() method returns a tuple containing: 
#1 - an empty string, 2 - an empty string, 3 - the whole string
txt = "I could eat bananas all day, bananas are my favorite fruit"
x = txt.rpartition("apples")
print(x)

#rsplit()	Splits the string at the specified separator, and returns a list
#Split a string into a list, using comma, followed by a space (, ) as the separator
txt = "apple, banana, cherry"
x = txt.rsplit(", ")
print(x) 

#Split the string into a list with maximum 2 items:
txt = "apple, banana, cherry"

# setting the maxsplit parameter to 1, will return a list with 2 elements!
x = txt.rsplit(", ", 1)
print(x) 


#rstrip()	Returns a right trim version of the string
#Remove any white spaces at the end of the string
txt = "     banana     "
x = txt.rstrip()
print("of all fruits", x, "is my favorite") 

#Remove the trailing characters if they are commas, periods, s, q, or w.
txt = "banana,,,,,ssqqqww....."
x = txt.rstrip(",.qsw")
print(x) 


#split()	Splits the string at the specified separator, and returns a list
#Split a string into a list where each word is a list item
txt = "welcome to the jungle"
x = txt.split()
print(x) 

#Split the string, using comma, followed by a space, as a separator
txt = "hello, my name is Peter, I am 26 years old"
x = txt.split(", ")
print(x) 

#Use a hash character as a separator
txt = "apple#banana#cherry#orange"
x = txt.split("#")
print(x) 

#Split the string into a list with max 2 items:
txt = "apple#banana#cherry#orange"
# setting the maxsplit parameter to 1, will return a list with 2 elements!
x = txt.split("#", 1)
print(x)

#splitlines()	Splits the string at line breaks and returns a list
#Split a string into a list where each line is a list item
txt = "Thank you for the music\nWelcome to the jungle"
x = txt.splitlines()
print(x)

#Split the string, but keep the line breaks:
txt = "Thank you for the music\nWelcome to the jungle"
x = txt.splitlines(True)
print(x) 

#startswith()	Returns true if the string starts with the specified value
#Check if the string starts with "Hello":
txt = "Hello, welcome to my world."
x = txt.startswith("Hello")
print(x) 

#Check if position 7 to 20 starts with the characters "wel":
txt = "Hello, welcome to my world."
x = txt.startswith("wel", 7, 20)
print(x) 

#strip()	Returns a trimmed version of the string
#Remove spaces at the beginning and at the end of the string
txt = "     banana     "
x = txt.strip()
print("of all fruits", x, "is my favorite") 

#Remove the leading and trailing characters:
txt = ",,,,,rrttgg.....banana....rrr"
x = txt.strip(",.grt")
print(x) 

#swapcase()	Swaps cases, lower case becomes upper case and vice versa
#Make the lower case letters upper case and the upper case letters lower case:
txt = "Hello My Name Is PETER"
x = txt.swapcase()
print(x)

#title()	Converts the first character of each word to upper case
#Make the first letter in each word upper case
txt = "Welcome to my world"
x = txt.title()
print(x) 

#Make the first letter in each word upper case:
txt = "Welcome to my 2nd world"
x = txt.title()
print(x) 

#Note that the first letter after a non-alphabet letter is converted into a upper case letter:
txt = "hello b2b2b2 and 3g3g3g"
x = txt.title()
print(x) 


#translate()	Returns a translated string
#eplace any "S" characters with a "P" character:
#use a dictionary with ascii codes to replace 83 (S) with 80 (P):
mydict = {83:  80}
txt = "Hello Sam!"
print(txt.translate(mydict))

#Use a mapping table to replace "S" with "P":
txt = "Hello Sam!"
mytable = str.maketrans("S", "P")
print(txt.translate(mytable)) 

#Use a mapping table to replace many characters:
txt = "Hi Sam!"
x = "mSa"
y = "eJo"
mytable = str.maketrans(x, y)
print(txt.translate(mytable)) 

#The third parameter in the mapping table describes characters that you want to remove from the string:
txt = "Good night Sam!"
x = "mSa"
y = "eJo"
z = "odnght"
mytable = str.maketrans(x, y, z)
print(txt.translate(mytable)) 

#The same example as above, but using a dictionary instead of a mapping table:
txt = "Good night Sam!"
mydict = {109: 101, 83: 74, 97: 111, 111: None, 100: None, 110: None, 103: None, 104: None, 116: None}
print(txt.translate(mydict)) 

#upper()	Converts a string into upper case
#Upper case the string:
txt = "Hello my friends"
x = txt.upper()
print(x) 

#zfill()	Fills the string with a specified number of 0 values at the beginning
#Fill the string with zeros until it is 10 characters long:
txt = "50"
x = txt.zfill(10)
print(x) 

#Fill the strings with zeros until they are 10 characters long:
a = "hello"
b = "welcome to the jungle"
c = "10.000"

print(a.zfill(10))
print(b.zfill(10))
print(c.zfill(10)) 



