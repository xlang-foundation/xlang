#List Comprehension - to create a new list based on the values of an existing list
fruits = ["kiwi", "banana", "apple", "cherry", "mango"]
newlist = []
print("before append: new list =")
print(newlist) 
for x in fruits:
  if "a" in x:
    newlist.append(x)


newlist.insert(2, "watermelon")
print("After append: new list =")
print(newlist) 
cntList = newlist.count()
szList = newlist.size()
print("count =", cntList)
print("size =", szList)
newlist.pop(1)
print("After pop: new list =")
print(newlist) 

print("index of Watermelon =", newlist.index("Watermelon"))

print("before sort, list =")
print(newlist) 
newlist.sort(True)
print("after sort, list =")
print(newlist) 

print ("----------------------------------------------")
# newnewlist = []
# newlist = [1,2,"test01"]
# newlist = ["kiwi", "banana", "apple", "cherry", "mango"]
print("original list =", newlist)
newnewlist = newlist.copy()
print("copy of list =", newnewlist)
print ("----------------------------------------------")


oldlist = ["orange","strawberry","pear"]
print("old list =", oldlist)
print("new list =", newlist)
newlist.extend(oldlist)
print("extended list =", newlist)
print ("----------------------------------------------")

print("before reverse, list =")
print(newlist) 
newlist.reverse()
print("after reverse, list =")
print(newlist)


