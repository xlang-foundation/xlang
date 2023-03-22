t1 = tensor([[1,2,3],[4,5,6],[7,8,9]],name="t1")
t2 = tensor([[10,20,30],[40,50,60],[70,80,90]],name="t2")
t3 =tensor([0.1,0.2,0.3],name="t3")
import CpuTensor as T
t4 =  t1*t2+t3*(t1*t3+t1*t2)
t_g = T.graph(t4)
print(t_g)
t_g.run()



