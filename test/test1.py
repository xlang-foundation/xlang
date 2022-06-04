def func1(i0:int,i1:int):
	x =i0
	y=i1
	z=x+y
	return z
t = time()
var1 =0
num =100000
for i in range(num):
	var1 =var1+ func1(i,100)
t2 = time()
print("timespend=",t2-t)