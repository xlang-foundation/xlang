breakpoint()
class base_class():
    prop0:str="prop0_value"
    def test():
        print("from base_class's test()")

class another_class():
    prop0:str="prop0_value"
    def test():
        print("from base_class's test()")

class test_class(base_class,another_class):
 prop1:int=1
 prop2:str='prop2'
 
 def constructor():
     print("test_class")
 def deconstructor():
     print("deconstructor")

 def method1(x,y):
     k = this.[prop1,prop2]
     print("method1->prop1:",k[0],",",k[1])
     return x+y+time()

 def method2(z):
     print("method2:",z)
     return rand()

x0 = test_class()
x0.prop1 =1
breakpoint("x0=",x0)
x0.prop2 ="calss x0"
x0.method1(100)
x0.prop2 ="class x0"
x1 = test_class()
x1.prop1 =2
x1.prop2 ="calss x1"
breakpoint("x1.prop2",x1.prop2)
x2 = test_class()
x2.prop1 =3
x2.prop2 ="calss x2"
print(x0.prop2)
x3 = test_class()
x3.prop1 =4
x3.prop2 ="calss x3"
x =[x0,x1,x2,x3]
ret = x.[method1,method2](1,2)
print("ret=",ret)




