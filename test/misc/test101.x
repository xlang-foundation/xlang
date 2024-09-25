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
srv = http.Server()
srv.OnConnect+= (name,port){
	print("event fired,srv.OnConnect->",name,":",port)
}
srv.test = 111*10
srv.OnConnect.wait(10,20)
print(srv.test)
srv.name = "http set prop to this"
x = srv.name
x+="->1000"
print(x)

srv.listen("localhost", 8080)
print("end")
