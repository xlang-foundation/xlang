from xlang_http import http
from xlang_os import fs
print("http Server Started")
srv = http.Server()
root = "c:/Dev/Cantor/factory"
print("root=${root}")
srv.get(".*",(req,res){
  path = req.path;
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
  params = req.params;
  pa = root+path;
  print("pa=${pa},mime=${mime}")
  f = fs.File(pa,openMode);
  f_size = f.size;
  if f_size >=0:
    data1 = f.read(f_size)
  else:
    data1 = "";
  f.close();
  #print("data=",data1);
  res.set_content(data1, mime);
  print("after set_content");
})

srv.get("/stop",(req,res){
  print("stop....")
  srv.stop();
})

srv.listen("::1", 8088)
print("Http Server Stopped")
