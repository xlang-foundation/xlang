#
# Copyright (C) 2024 The XLang Foundation
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# <END>

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
