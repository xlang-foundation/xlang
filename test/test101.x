from xlang_http import http
srv = http.Server()
srv.test = 111*10
print(srv.test)
srv.name = "http set prop to this"
x = srv.name
x+="->1000"
print(x)

srv.listen("localhost", 8080)
print("end")
