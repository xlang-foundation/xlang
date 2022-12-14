def func1(i0:int,i1:int):
	x =i0
	y=i1
	z=x+y
	return z
num =10
def taskfunc(n):
	var1 =0
	for i in range(num):
		var1 =var1+ func1(i,100)
		print("tid:",threadid(),",sum:",var1)
	return var1
t = time()
print("main tid:",threadid())
taskfunc.taskrun(num)
taskfunc(num)
t2 = time()
print("timespend=",t2-t)
sleep(10000)
