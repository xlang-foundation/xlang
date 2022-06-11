class test_class():
 prop1:int=1
 prop2:str='prop2'
 
 def constructor():
     print("test_class")
 def deconstructor():
     print("deconstructor")

 def method1(x,y):
     print("method1:",x,",",y)

 def method2(z):
     print("method2:",z)

x = test_class()
x.method1(1,2)
