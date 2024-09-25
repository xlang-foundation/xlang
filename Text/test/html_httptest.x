import html
from xlang_http import http
client = http.Client("https://www.oxfordlearnersdictionaries.com")
client.post("/us/definition/english/transcend?q=transcending")
s = client.body
x = html.loads(s)

client.get("/us/definition/english/enormously?q=enormously")
s2 = client.body
x2 = html.loads(s2)

print("end")