num =10
def func1(v):
 k =1
 k=k+1
 print(v)
s ="this is a string"
print("Test Loop:",num)
sum =0
t1 = time()
for i in range(num):
 sum = sum+i
 sum = sum+3
 sum = sum-3
t2 = time()
t2 = (t2-t1)
print("sum=",sum)
print("All->time spend:",t2)
