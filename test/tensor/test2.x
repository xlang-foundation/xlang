'''
# add a number to a tensor
t1 = tensor([[1,2,3],[4,5,6],[7,8,9],[10,11,12]])
print("t1 =", t1)
import CpuTensor as T
#t11 = t1 + 10
t11 = 10 + t1
#t1+=10
#t1 =t1+10
#t1.Add(10)
t_g = T.graph(t11)
print(t_g)
print("before graph run, t1+10=", t11)
t_g.run()
#print("after graph, b.t1+10=", t1)
print("after graph run, t1+10=", t11)

'''
#to add a tensor to a tensor
t1 = tensor([[1,2,3],[4,5,6],[7,8,9],[10,11,12]])
print("t1 =", t1)
t2 = tensor([[10,20,30],[40,50,60],[70,80,90], [100,110,120]])
print("t2 =", t2)
import CpuTensor as T
t11 = t1+t2
print("before graph run, t1+t2=", t11)
t_g = T.graph(t11)
print(t_g)
t_g.run()
print("after graph run, t1+t2=", t11)
'''

#t11 = t2-t1
#print("a.t2-t1=", t11)
#print("b.t2-t1=", t11)

#t3 = tensor([[51,52,53],[54,55,56],[57,58,59],[60,61,62]])
#t4 = tensor([[3,2,1],[6,7,8],[8,6,4],[2,0,3]])
#print("t3 =",t3)
#print("t4 =", t4)
#t3 -= t4
#print("t3-t4 =", t3)

#import CpuTensor as T
#t3 =  t1*T.permute([2,1,0])*t2
#t_g = T.graph(t3)
#t_g.run()


#t1 = tensor([[1,2,3],[4,5,6],[7,8,9]],name="t1")
#t2 = tensor([[10,20,30],[40,50,60],[70,80,90]],name="t2")
#t3 =tensor([0.1,0.2,0.3],name="t3")
#import CpuTensor as T
#t4 =  t2+t1*T.permute([1,0])*T.div()*t3
#t5 = t4+t3*(t1*t3+t1*t2)
#t_g = T.graph(t4,t5)
#print(t_g)
#t_g.run()
