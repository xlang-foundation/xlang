from xlang_http import http

srv = http.Server()


@srv.route('/first/<name>/<number>')
def SecondPage(name,number):
	print("in second page",name)
	mime = "text/html"
	#res.set_content("<h1>"+name+"</h1>", mime)
	content = "
		<div>
			<h1>${name}</h1>
			<h1>${number}</h1>
			<button>Test</button>
		</div>
	"
	return [content, mime]


print("http Server Started")
srv.listen("::1", 8088)
print("Http Server Stopped")

