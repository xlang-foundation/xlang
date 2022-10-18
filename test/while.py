x =0
num = 1000
sum =0
t1 = time()
while x<num:
    sum =sum+x
    x =x+1
t2 = time()
t2 = (t2-t1)
print("sum=",sum)
print("All->time spend:",t2)
print("End While test")