def New_Test_Func(x,y):
	print("from New_Test_Func,xy=",x,y)
def test_decor(x,y)
	origin("from test_decor",20)
	print("inside test_decor")
	return New_Test_Func
def test_decor2(x,y)
	origin("from test_decor2",30)
	print("inside test_decor2")
@test_decor(10,10)
@test_decor2(2,2)
def test_func(x,y):
	print("x=",x,",y=",y)
test_func(10,100)
print("end")