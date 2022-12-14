x=[](size=5,init='rand(1.0,100.0)')
num =x.size()

def test(i):
  print("test->",i)

test.setattr("func_name","function also can have attributes")
print(test.getattr("func_name"))
x.each((i,item,passIn){
    extern num;
	num1 = num+i;
	#num = num1;
	test(num1);
	print(num1,",i=",i,",item=",item,",PassIn->",passIn);
},"passed in param 1")
setattr(x,a2,'this is an attribute for a2')
print(x.getattr(a2))
x.setattr('a1','this is an attribute for a1')
print(x.getattr(a1))
x.delattr(a1);
print("after del:",x.getattr(a1))
