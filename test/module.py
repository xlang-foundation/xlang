import http
#import test_class(12) as t,numpy(1,2,3) as np
#from python import numpy as np,os,cv2 as cv 
print("http Server Started")
srv = http.Server()
handler = (res){
  print("Hi from http server")
  res.set_content("Hello World!\n","text/plain");
}
srv.get("/hi",handler)

handler2 = (res){
  print("Slow from http server")
  sleep(1000)
  res.set_content("Slow...\n", "text/plain");
}
srv.get("/slow",handler2)

handler3 = (res){
  print("stop....")
  srv.stop();
}
srv.get("/stop",handler3)

srv.listen("localhost", 8080)
print("Http Server Stopped")
