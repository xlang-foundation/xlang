import datetime
def Func2(r):
    for i in range(r):
        print("in Func2,i=",i)

def innerFunc(x,y,z):
    print("xyz=",x,y,z)
    Func2(3)
def test(x):
 print("x=",x)
 y = x
 x = y+1
 z = ["from python test",x,y]
 innerFunc(x,y,z)
 return {"z":z,"x":x,"y":y}

start_time = datetime.datetime.now()
print("in python module:pymodule:",start_time)