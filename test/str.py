import fs
filename = "C:/Dev/Data/loghub/Windows/Windows_2k.log"
filename2 = "C:/Dev/english-words/words.txt"
f = fs.File(filename,"r")
##C:\Dev\Data\loghub\Windows\Windows_2k.log
data = f.read(f.size())
l = data.split()
print(l[10])
print(l[100])
print("end")

#x ="abcdef"
#x|(y){print(y);}|(){print(x);}
#x.find("cd")|y|y-1|x.slice(y,4)|z|k|l|m
#print("k=",k,",z=",z,",y=",y,",l=",l,",m=",m)

"""
x|(z){
    print(z);return "return from pipe";
}|y
print(y)

import fs
f = fs.File("c:/temp/test.json","r")
data = f.read(f.size())
#print(data)
f.close()
pos = data.find("glossary")
pos2 = data.find("GlossList")
print(data.slice(pos,pos2))
x ="0123456789"
#x *=10
x +="abcd"
print("x.size=",x.size())
k = x.slice(0,10)
print("k=",k)
z = "567"
y = x.find(z,12)

print("x find ",z,",at:",y)
"""