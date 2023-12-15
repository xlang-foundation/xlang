
import time
num =1000000000
print("Test Loop with XLang Jit:",num)
@jit
def sum_all(toNum:unsigned long long)->unsigned long long:
    unsigned long long sum = 1;
    for(unsigned long long i=0;i<toNum;i++)
    {
        sum += i;
    }
    return sum;
print("start")
@jit
def test_second(m:int,n:int)->int:
    return m+n;

t1 = time.time()
sum = sum_all(num)
t2 = time.time()
t2 = (t2-t1)
print("sum=",sum)
k = test_second(1,2)
print("k=",k)
print("All->time spend:",t2)

print("end")
