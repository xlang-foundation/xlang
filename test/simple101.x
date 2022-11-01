from xlang_http import http
from xlang_os import fs
f = fs.File("data/2.mp3","wb")

c = http.Client("http://s3.amazonaws.com")
c.get("/audio.vocabulary.com/1.0/us/I/1VXPPAPE63FQ5.mp3")
s = c.status
b = c.body
f.write(b)
f.close()
print("end")

