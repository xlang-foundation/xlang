breakpoint()
def func1(x1):
    print("inside func1,x1=",x1)
    return x1+1000
def func2(x1):
    print("inside func2******,x1=",x1)
    return x1*123
sleep(10,time=100)
f = [func1,func2].sleep(123,time=500)
print("taskrun,ret=",f)
x=1
y=2
z =func1.taskrun(x-2)*(x+1)+y*10
print("x=",x,",y=",y,",z=",z)