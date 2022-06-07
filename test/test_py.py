import datetime
start_time = datetime.datetime.now()

def func1(x,y):
   print("func1")
f =func1
f(1,1)
x = 0
num  =10000000
for i in range(num):
   x += i
   x = x+3
   x = x-3

end_time = datetime.datetime.now()

time_diff = (end_time - start_time)
execution_time = time_diff.total_seconds() * 1000

print("num=",num,execution_time,",sum=",x)
print("end")
