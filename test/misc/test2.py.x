i=10
sum =0
sum+=10

import time
num =30000000
print("Test Loop:",num)
sum =0
t1 = time.time()
for i in range(num):
 sum +=i

t2 = time.time()
t2 = (t2-t1)
print("sum=",sum)
print("All->time spend:",t2)
