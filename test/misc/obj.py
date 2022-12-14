
#test
"""
def test(x:int=1,y=2):
    print("x=",x,",y=",y)
    return x+y
k = test(100,100)
print("return from func:test:",k)
"""
#[].func1().func2(1)
"""
num =100

point ={x:0,y:0}

x =[](size=num,type='int',
    init=[1,2,3,4]
    ,shape=[10,10,10])
#x.select("*x").move(10,10).turn()>>y
x[0][1] =300
for i in range(num):
 print(x[i][1])
"""
"""
num =1000000
for i in range(num):
 x[i] =i
for i in range(10):
 x[i] = x[i]
 print(x[i])
"""


