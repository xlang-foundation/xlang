m1 ="100"
m2 = 1992

m1 +=10

def test(x1,x2):
  extern m1,m2
  print("inside test m1=",m1,",m2=",m2)
  m1 = x1+x2
  m2 += x2
print("before test func called,m1=",m1,",m2=",m2)
test(11,1000)
print("after test func called,m1=",m1,",m2=",m2)


