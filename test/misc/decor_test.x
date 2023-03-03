x =(3>1)() #expresion
y = (x+7)(10) # still expresion
print("x=",x,"y=",y)
class test_class():
	def func1(x,y):
		print("from test_class.func1,",x(),y())# must use (), x,y passed in with expr


def New_Test_Func(x,y):
	print("from New_Test_Func,xy=",x,y)
def test_decor(x,y)
	origin("from test_decor",20)
	print("inside test_decor")
	return New_Test_Func
def test_decor2(x,y)
	origin("from test_decor2",30)
	print("inside test_decor2")
tc = test_class()
k=10.2
print("k=",k)
@tc.func1(3+k,14)
@test_decor(10,10)
@test_decor2(2,2)
def test_func(x,y):
	print("x=",x,",y=",y)
test_func(10,100)
print("end")