import CpuTensor as T
t1 = tensor([[1,2],[3,4], [5,6]])
print("t1 =", t1)
t2 = tensor([[10,20],[30,40]])
print("t2 =", t2)

print ("***********************************************************************")
#a tensor multiply with another tensor with different dimensions
t11 = t1*T.matmul()*t2
print("before graph run, t1*t2=", t11)
t_g = T.graph(t11)
print("graph is ", t_g)
t_g.run()
print("after graph run, t1*t2=", t11)
#expected sum is = [[70,100],[150,220],[230,340]]

print ("***********************************************************************")
#a tensor multiply with a vector
t2 = tensor([1,2])
print("t1 =", t1)
print("t2 =", t2)
t11 = t1*T.matmul()*t2
print("before graph run, t1*t2=", t11)
t_g = T.graph(t11)
print("graph is ", t_g)
t_g.run()
print("after graph run, t1*t2=", t11)
#expected sum is = [[70,100],[150,220],[230,340]]
