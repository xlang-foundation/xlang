#to add a tensor to a tensor with different dimensions
t1 = tensor([[[1,2],[3,4]],[[5,6],[7,8]],[[9,10],[11,12]]])
print("t1 =", t1)
t2 = tensor([[1,2],[3,4]])
print("t2 =", t2)
import CpuTensor as T
t11 = t1*t2
print("before graph run, t1+t2=", t11)
t_g = T.graph(t11)
print(t_g)
t_g.run()
print("after graph run, t1+t2=", t11)

#expected sum is = [[[1,4],[9,16]], [[5,12],[21,32]],[[9,20],[33,48]]]