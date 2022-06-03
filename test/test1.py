def func1(i0:int,i1:int):
	x =i0
	y=i1
	z=x+y
	return z
var1 =0
for i in range(1000):
	var1 =var1+ func1(i,100)
print("var1=",var1)