from xlang_http import http

srv = http.Server()

out_var ="out side variable"
@srv.route("^/$")
def IndexPage():
	x = out_var
	mime = "text/html"
	content = "
		<div>
			<h1>Index Page</h1>
		</div>
	"
	return [content, mime]

@srv.route('/first/<name>/<number>')
def FirstPage(name,number):
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

@srv.route('/second/<name>/<number>')
def SecondPage(name,number):
	mime = "text/html"
	content = "
		<div>
			<h1>${name}</h1>
			<textarea>this is from url:${number}</textarea>
			<button>Test</button>
			</div>
	"
	return [content, mime]

print("http Server Started")
srv.listen("::1", 8088)
print("Http Server Stopped")

