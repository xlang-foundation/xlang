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
     this.prop1 =100
     print("method1:",x,",",y)
     return x+y

 def method2(z):
     print("method2:",z)
     return z

x0 = test_class()
x1 = test_class()
x2 = test_class()
x =[x0,x1,x2].[method1,method2](1,2)

#x.[method1,method2](1,2)



