from xlang_http import http
http.test = 100
srv = http.Server
srv.listen("localhost", 8080)
print("end")
