import CpuTensor as T
t1 = tensor([[1,2,3],[4,5,6],[7,8,9]])
t2 = tensor([[10,20,30],[40,50,60],[70,80,90]])

print("t1 =", t1)
print("t2 =", t2)
print ("***************************************************************")

#to divide by a number
t3 = t2/2
#t1 =t1*10
#t1 = 2*t1
t_g = T.graph(t3)
t_g.run()
print("t2/2=", t3)
#expected result 
#Tensor(size=(3,3),[20,40,60,80,100,120,140,160,180])
print ("***************************************************************")

t3 = 90/t1
#t1 =t1*10
#t1 = 2*t1
t_g = T.graph(t3)
t_g.run()
print("90/t1=", t3)
#expected result 
#Tensor(size=(3,3),[20,40,60,80,100,120,140,160,180])
print ("***************************************************************")

#to divide by a number
t2/=2
#t1 =t1*10
#t1 = 2*t1
t_g = T.graph(t2)
t_g.run()
print("t2/2=", t2)
#expected result 
#Tensor(size=(3,3),[20,40,60,80,100,120,140,160,180])
print ("***************************************************************")

#to divide a tensor by a tensor
t3 = t2 / t1
#t1*t2
#t1.MUL(t2)
print(t3)
t_g = T.graph(t3)
t_g.run()
print("t2/t1=", t3)
#expected result 
#Tensor(size=(3,3),[10,40,90,160,250,360,490,640,810])
print ("***************************************************************")


