kk =10
z = [](size=5,init='rand(1.0,100.0)')
y={"a":"a1","b":10,"C":"c1","Z":z}
x =["string_value",10.2,1,3,y]


def test_sleep(x,y):
    print("after sleep,x=",x,"y=",y)
print("before call")
test_sleep.sleep(100,200,time=1000)
print("after call")

from xlang_http import http
from xlang_os import fs

c = http.Client("http://s3.amazonaws.com")
c.get("/audio.vocabulary.com/1.0/us/I/1VXPPAPE63FQ5.mp3")
s = c.status
if s == 200:
    f = fs.File("data/2.mp3","wb")
    b = c.body
    f.write(b)
    f.close()
else:
    print("http return error:",s)
print("end")

