loop = 1000000
import time
start = time.time()
okValue = "OK?"
ElseValue ="100"
# events = [ okValue if i%5 ==0 else 100 for i in range(loop) if i % 3 == 0]
# XLang
events = []
for i in range(loop):
    if i % 3 == 0:
        if i % 5 == 0:
            events+=okValue
        else:
            events+=ElseValue

end = time.time()
elapsed = (end - start) * 1000
avg = elapsed / loop * 1000000
print(elapsed,"-",avg)