

A = set("a", "e", "i", "o", "u")
#A = set(("a", "e", "i", "o", "u"))
#A = set()
print(A)
print("type of A =",type(A)) 
print(A.Size())

#print(len(A)) 
#A.Remove("i")
A -= "i"
#A -= {"i"}
#A -= set("i")
print(A)
print(A.Size())

#A.Add ("z")
A += "z"
#A += "z"
print(A)
print(A.Size())
#print(A.Size())
#print(len(A)) 

#B = {"red", "orange", "yellow", "green"}
B = {1,2,3}
#B = {1.1,2.2,3.3}
print("type of B =",type(B)) 
print(B)
#print(len(B)) 
#C = {"blue", "indigo", "violet"}
#B.update(C)
##print(B)
#print(len(B)) 


#B = {"red", "orange", "yellow", "green"}
#B.add ("blue")
#print(B)

#duplicate not allowed
#thisset = {"apple", "banana", "cherry", "apple"}
#print(thisset)

