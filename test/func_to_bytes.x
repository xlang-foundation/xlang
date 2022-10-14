from xlang_http import http
def embeded_func01(m,n):
	http.name ="from embeded_func01"
	print("m=",m,",n=",n)
	return m+n
def test_func(x,y):
	http.name ="from embeded_func01"
	z =embeded_func01(x,y)
	print("x=",x,",y=",y)
	return z
out = bytes(test_func)
runbytecode(out);
print("end")
