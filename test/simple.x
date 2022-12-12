f = (x,y:int)
{
    x = x+y;
    print("x=${x}");
    return x;
}(10,20)
x =-100.111
y =0.111
def f2(x,y):
    z = x+y
    print("inside f2,x+y=",z)
    return z
z = f2(x,y)
print("End,f=",f,",z=",z)