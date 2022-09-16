from xlang_http import http
srv = http.Server()
srv.OnConnect+= (name,port){
	print("event fired,srv.OnConnect->",name,":",port)
}
srv.test = 111*10
srv.OnConnect.wait(10,20)
print(srv.test)
srv.name = "http set prop to this"
x = srv.name
x+="->1000"
print(x)

srv.listen("localhost", 8080)
print("end")
