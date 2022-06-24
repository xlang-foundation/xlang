import http
import fs
print("http Server Started")
srv = http.Server()
srv.get("/hi",(req,res){
  params = req.get_params();
  f = fs.File(params["file"],"rb")
  data = f.read(f.size())
  f.close()
  res.set_content(data, "text/html");
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
