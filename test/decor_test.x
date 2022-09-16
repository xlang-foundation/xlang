@test_decor(10,10)
def test_func(x,y):
	@inside_decor(20,20)
	print("x=",x,",y=",y)
@cold_decor(20,20)
test_func(10,100)
@car_decor(30,30)
print("end")