from xlang_http import http
client = http.Client("https://www.oxfordlearnersdictionaries.com")
client.post("/us/definition/english/transcend?q=transcending")
s = client.body
print("fetching done:",s)

client.get("/us/definition/english/enormously?q=enormously")
s2 = client.body
print("fetching done-2:",s2	)
print("end")