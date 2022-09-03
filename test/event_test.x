x=[1,2,4]
x+="this is a test"
print(x,"size=",x.size())
x.remove(0)
print(x)

rint("test event system")
on("event1",(evt){
	print("event1 fire");
})

on("event1",(evt){
	print("event1 fire 2");
})

fire("event1",1,2,3)