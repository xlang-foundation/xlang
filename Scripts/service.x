import http
import fs
print("http Server Started")
srv = http.Server()
root = "C:/Dev/Cantor/factory"
srv.get(".*",(req,res){
  path = req.get_path();
  print(path);
  pos = path.rfind(".");
  mime = "text/html";
  openMode ="r"
  if pos >0:
	ext =path.slice(pos+1,path.size())
	print("ext=${ext}")
	if ext == "js":
	  mime = "text/javascript"
	elif ext == "jpg":
	  mime = "image/jpeg"
	  openMode ="rb"

  params = req.get_params();
  pa = "C:/Dev/Cantor/factory";
  pa +=path;
  f = fs.File(pa,openMode)
  data = f.read(f.size())
  f.close()
  res.set_content(data, mime);
})

srv.get("/test",(req,res){
  params = req.get_params();
  f = fs.File(params["file"],"rb")
  data = f.read(f.size())
  f.close()
  res.set_content(data, "image/png");
})

srv.get("/stop",(req,res){
  print("stop....")
  srv.stop();
})

srv.listen("localhost", 8080)
print("Http Server Stopped")
