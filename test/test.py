breakpoint()

class base_class():
    prop0:str="prop0_value"
    obj1=0
    def func1(x1):
        print("Tid=",threadid(),",inside func1,x1=",x1,",prop0=",this.prop0)
        sleep(10000)
        print("Tid=",threadid(),",after sleep")
        return x1+1000
def func2(x1):
    print("inside func2******,x1=",x1)
    return x1*123
c1 = base_class()
c1.obj1 = base_class()
f = [c1].[obj1].func1.taskrun(123)
print("Main Tid=",threadid(),",taskrun,ret=",f)
x=1
y=2
z =func2(x-2)*(x+1)+y*10
print("x=",x,",y=",y,",z=",z)
sleep(20000)