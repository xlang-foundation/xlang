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

import datetime
def Func2(r):
    for i in range(r):
        print("in Func2,i=",i)

def innerFunc(x,y,z):
    print("xyz=",x,y,z)
    Func2(3)
def test(x):
 print("x=",x)
 y = x
 x = y+1
 z = ["from python test",x,y]
 innerFunc(x,y,z)
 return {"z":z,"x":x,"y":y}

start_time = datetime.datetime.now()
print("in python module:pymodule:",start_time)