t1 = tensor(3.1415,shape=[10,10])
t2 = t1.reshape([5,20])
t3 = t2.reshape([100])
t1[1][1] =20
print(t1)
print(t2)
print(t3)
