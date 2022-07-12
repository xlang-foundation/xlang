x=[](size=378,init='rand(1.0,100.0)')
x[0]=0
x[1]=1
for i in range(100):
    x[i] = i*i+0.51*i
sleep(10000)
x+=[](size=100,init='rand(1990.0,2999.0)')
print(x)


