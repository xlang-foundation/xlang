x=[](size=10000,init='rand(1.0,100.0)')
num =1

def test():
  print("test")
x.each((i,item,passIn){
	num1 = num+1;
	num = num1;
	print(num,",i=",i,",item=",item,",PassIn->",passIn);
},"passed in param 1")
setattr(x,a2,'this is an attribute for a2')
print(x.getattr(a2))
x.setattr('a1','this is an attribute for a1')
print(x.getattr(a1))
x.delattr(a1);
print("after del:",x.getattr(a1))
