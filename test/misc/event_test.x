x=[1,2,4]
x+="this is a test"
print(x,"size=",x.size())
x.remove(0)
print(x)

print("test event system")
on("event1",(x1,y,z){
	print("event1 fire",x1,y,z);
})

on("event1",(x,y,z){
	print("event1 fire 2",x1,y,z);
})

fire("event1",10,-20,-30)