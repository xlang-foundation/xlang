from xlang_http import http
srv = http.Server()
srv.test =100
srv.name = "shawn"
x = srv.test
srv.listen("localhost",3111)
print("end")