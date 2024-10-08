﻿#
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

x=[1,2,4]
x+="this is a test"
print(x,"size=",x.size())
x.remove(0)
print(x)

print("test event system")
on("event1",(x1,y,z){
	print("event1 fire",x1,y,z);
})

on("event1",(x,y,z){
	print("event1 fire 2",x1,y,z);
})

fire("event1",10,-20,-30)