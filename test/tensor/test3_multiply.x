import CpuTensor as T


print("*******************************************")
#to multiply a number
t1 = tensor([[1,2,3],[4,5,6],[7,8,9]])
print("t1 =", t1)
t3 = t1*20
#t3 = 2*t1
print(t3)
t_g = T.graph(t3)
t_g.run()
print("t1*20 = ",t3)
#expected result 
#Tensor(size=(3,3),[20,40,60,80,100,120,140,160,180])

print("*******************************************")
#to multiply a tensor to a tensor
t1 = tensor([[1,2,3],[4,5,6],[7,8,9]])
t2 = tensor([[10,20,30],[40,50,60],[70,80,90]])
print("t1 =", t1)
print("t2 =", t2)
t3 = t1 * t2
#t1*t2
#t1.MUL(t2)
print(t3)
t_g = T.graph(t3)
t_g.run()
print("t1*t2=", t3)
#expected result 
#Tensor(size=(3,3),[10,40,90,160,250,360,490,640,810])


print("*******************************************")
#t1 = tensor([[1,2,3],[4,5,6],[7,8,9],[10,11,12]])
#t2 = tensor([[10,20,30],[40,50,60],[70,80,90], [100,110,120]])

#t3 = tensor([[51,52,53],[54,55,56],[57,58,59],[60,61,62]])
#t4 = tensor([[3,2,1],[6,7,8],[8,6,4],[2,0,3]])
#print("t3 =",t3)
#print("t4 =", t4)
#t3 -= t4
#print("t3-t4 =", t3)

#t3 =  t1*T.permute([2,1,0])*t2
#t_g = T.graph(t3)
#t_g.run()

#t72 = tensor([[1,0,1], [2,1,1], [0,1,1],[1,1,2]])
#t90 = tensor([[1,2,1],[2,3,1],[4,2,2]])
#print("t72 =", t72)
#print("t90 =", t90)

print("*******************************************")
t72 = tensor([1,2,3,4,5])
t90 = tensor([[1,2,3,4,5],[2,4,6,8,10],[3,6,9,12,15]])
print("t72 =", t72)
print("t90 =", t90)
t3 = t90*t72
t_g = T.graph(t3)
t_g.run()
print("t90*t72 =", t3)
#expected result
#t3 = Tensor(size=(3,5),[1,4,9,16,25,2,8,18,32,50,3,12,27,48,75])