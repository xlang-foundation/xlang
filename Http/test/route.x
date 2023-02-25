from xlang_http import http

srv = http.Server()

@srv.route('/first/<name>/<number>')
def FirstPage(name,number):
	print("in first page",name,number)
	mime = "text/html"
	#res.set_content("<h1>"+number+"</h1>", mime);

@srv.route('/first/<name>')
def SecondPage(name):
	print("in second page",name)
	mime = "text/html"
	#res.set_content("<h1>"+name+"</h1>", mime)
	content = """
		<div>
			<h1>${name}</h1>
			<button>Test</button>
		</div>
	"""
	return [content, mime]


print("http Server Started")
srv.listen("::1", 8088)
print("Http Server Stopped")

