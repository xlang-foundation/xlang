import xlang
http = xlang.importModule("http", fromPath="xlang_http")

srv = http.Server()

@srv.route("/hello",Serialization=False)
def hello(req,res):
    return "Hello World"

print("Server starting on port 8080...")
srv.listen("0.0.0.0", 8080)
