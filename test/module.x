import http
#import test_class(12) as t,numpy(1,2,3) as np
#from python import numpy as np,os,cv2 as cv 
print("http Server Started")
srv = http.Server()
srv.get("/hi",(req,res){
  path = req.get_path();
  print("path=${path}");
  print("Hi from http server")
  res.set_content("Hello World!\n","text/plain");
})

srv.get("/slow",(req,res){
  print("Slow from http server")
  sleep(1000)
  res.set_content("Slow...\n", "text/plain");
})

srv.get("/stop",(req,res){
  print("stop....")
  srv.stop();
})

srv.listen("localhost", 8080)
print("Http Server Stopped")
