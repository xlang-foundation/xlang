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
