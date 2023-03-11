t1 = tensor([[1,2,3],[4,5,6],[7,8,9]])
t2 = tensor([[10,20,30],[40,50,60],[70,80,90]])
import CpuTensor as T
t3 = t1*T.add()*t2
T.get(t3,t2)


