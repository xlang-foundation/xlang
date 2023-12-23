import time
num =100000000
print("Test Loop:",num)
sum =1
t1 = time.time()
for i in range(num):
  sum += i
  
t2 = time.time()
t2 = (t2-t1)
print("sum=",sum)
print("All->time spend:",t2)
