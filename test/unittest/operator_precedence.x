#Operator Precedence

#Precedence -() Parentheses has the highest precedence
print((6 + 3) - (6 + 3))

"""
Parenthesis have the highest precedence, and need to be evaluated first.
The calculation above reads 9 - 9 = 0
"""

#Precedence - ** Exponentiation
print(100 - 3 ** 3)

"""
Exponentiation has higher precedence than subtraction, and needs to be evaluated first.
The calculation above reads 100 - 27 = 73
"""

#Precedence - +x  -x  ~x 	Unary plus, unary minus, and bitwise NOT
print(100 + ~3)

"""
Bitwise NOT has higher precedence than addition, and needs to be evaluated first.
The calculation above reads 100 + -4 = 96
"""

#Precedence - *  /  //  % 	Multiplication, division, floor division, and modulus
print(100 + 5 * 3)

"""
Multiplication has higher precedence than addition, and needs to be evaluated first.
The calculation above reads 100 + 15 = 115
"""

#Precedence - +  - 	Addition and subtraction
print(100 - 5 * 3)

"""
Subtraction has a lower precedence than multiplication, and we need to calculate the multiplication first.
The calculation above reads 100 - 15 = 85
"""

#Precedence - <<  >> 	Bitwise left and right shifts
print(8 >> 4 - 2)

"""
Bitwise right shift has a lower precedence than subtraction, and we need to calculate the subtraction first.
The calculation above reads 8 >> 2 = 2

More explanation:
The >> operator moves each bit the specified number of times to the right. Empty holes at the left are filled with 0's.

If you move each bit 2 times to the right, 8 becomes 2:
 8 = 0000000000001000
becomes
 2 = 0000000000000010

Decimal numbers and their binary values:
 0 = 0000000000000000
 1 = 0000000000000001
 2 = 0000000000000010
 3 = 0000000000000011
 4 = 0000000000000100
 5 = 0000000000000101
 6 = 0000000000000110
 7 = 0000000000000111
 8 = 0000000000001000
 9 = 0000000000001001
10 = 0000000000001010
11 = 0000000000001011
12 = 0000000000001100
"""

#Precedence - & 	Bitwise AND
print(6 & 2 + 1)

"""
Bitwise AND has a lower precedence than addition, and we need to calculate the addition first.
The calculation above reads 6 & 3 = 2

More explanation:
The & operator compares each bit and set it to 1 if both are 1, otherwise it is set to 0:

6 = 0000000000000110
3 = 0000000000000011
--------------------
2 = 0000000000000010
====================

Decimal numbers and their binary values:
0 = 0000000000000000
1 = 0000000000000001
2 = 0000000000000010
3 = 0000000000000011
4 = 0000000000000100
5 = 0000000000000101
6 = 0000000000000110
7 = 0000000000000111
"""

#Precedence - ^ 	Bitwise XO
print(6 ^ 2 + 1)

"""
Bitwise XOR has a lower precedence than addition, and we need to calculate the addition first.
The calculation above reads 6 ^ 3 = 5

More explanation:
The ^ operator compares each bit and set it to 1 if only one is 1, otherwise (if both are 1 or both are 0) it is set to 0:

6 = 0000000000000110
3 = 0000000000000011
--------------------
5 = 0000000000000101
====================

Decimal numbers and their binary values:
0 = 0000000000000000
1 = 0000000000000001
2 = 0000000000000010
3 = 0000000000000011
4 = 0000000000000100
5 = 0000000000000101
6 = 0000000000000110
7 = 0000000000000111
"""


#Precedence - | 	Bitwise OR
print(6 | 2 + 1)

"""
Bitwise OR has a lower precedence than addition, and we need to calculate the addition first.
The calculation above reads 6 | 3 = 7

More explanation:
The | operator compares each bit and set it to 1 if one or both is 1, otherwise it is set to 0:

6 = 0000000000000110
3 = 0000000000000011
--------------------
7 = 0000000000000111
====================

Decimal numbers and their binary values:
0 = 0000000000000000
1 = 0000000000000001
2 = 0000000000000010
3 = 0000000000000011
4 = 0000000000000100
5 = 0000000000000101
6 = 0000000000000110
7 = 0000000000000111
"""


#Precedence - ==  !=  >  >=  <  <=  is  is not  in  not in  	Comparisons, identity, and membership operators
print(5 == 4 + 1)

"""
The "like" comparison has a lower precedence than addition, and we need to calculate the addition first.
The calculation above reads 5 == 5 = True
"""

#Precedence - not 	Logical NOT
print(not 5 == 5)

"""
The logical NOT operator has a lower precedence than "like" comparison, and we need to calculate the comparison first.
The calculation above reads: not True = False
"""

#Precedence - and 	AND
print(1 or 2 and 3)

"""
The and operator has a higher precedence than or, and we need to calculate the and expression first.
The calculation above reads: 1 or 3 = 1
"""

#Precedence - or 	OR
print(4 or 5 + 10 or 8)

"""
The or operator has a lower precedence than addition, and we need to calculate the addition first.
The calculation above reads: 4 or 15 or 8 = 4
"""

#Precedence - If two operators have the same precedence, the expression is evaluated from left to right.
print(5 + 4 - 7 + 3)
 
#Precedence - 


