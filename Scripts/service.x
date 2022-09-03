#define XX  R"(
from xlang_http import http
from xlang_os import fs
print("http Server Started")
srv = http.Server()
root = "c:/Dev/Cantor/factory"
print("root=${root}")
srv.get(".*",(req,res){
  path = req.get_path();
  print("path=${path}");
  pos = path.rfind(".");
  mime = "text/html";
  openMode ="r";
  if pos >0:
    ext =path.slice(pos+1,path.size())
	print("ext=${ext}")
	if ext == "js":
	  mime = "text/javascript"
	elif ext == "jpg":
	  mime = "image/jpeg"
	  openMode ="rb"
  params = req.get_params();
  pa = root+path;
  print("pa=${pa}")
  f = fs.File(pa,openMode);
  f_size = f.size();
  if f_size >=0:
    data = f.read(f_size)
  else:
    data=""
  f.close();
  res.set_content(data, mime);
})

srv.get("/stop",(req,res){
  print("stop....")
  srv.stop();
})

srv.listen("::1", 8088)
print("Http Server Stopped")
#)"