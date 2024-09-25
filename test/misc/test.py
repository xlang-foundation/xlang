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

class base_class():
    prop0:str="prop0_value"
    obj1=0
    def func1(x1):
        this.prop0 = x1
        print("Tid=",threadid(),",inside func1,x1=",x1,",prop0=",this.prop0)
        sleep(10000)
        print("Tid=",threadid(),",after sleep")
        return x1+1000
def func2(x1):
    print("inside func2******,x1=",x1)
    return x1*123
c1 = base_class()
c1.obj1 = base_class()
f = [c1].[obj1].func1.taskrun(1234)
breakpoint()
print("Main Tid=",threadid(),",taskrun,ret=",f)
f = [c1].[obj1].func1(12)
x=1
y=2
z =func2(x-2)*(x+1)+y*10
print("x=",x,",y=",y,",z=",z)
sleep(20000)