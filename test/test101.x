from xlang_http import http
http.test = 111
print(http.test)
http.name = "http set prop to this"
x = http.name
print(x)

srv = http.Server
srv.listen("localhost", 8080)
print("end")
