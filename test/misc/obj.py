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

#test
"""
def test(x:int=1,y=2):
    print("x=",x,",y=",y)
    return x+y
k = test(100,100)
print("return from func:test:",k)
"""
#[].func1().func2(1)
"""
num =100

point ={x:0,y:0}

x =[](size=num,type='int',
    init=[1,2,3,4]
    ,shape=[10,10,10])
#x.select("*x").move(10,10).turn()>>y
x[0][1] =300
for i in range(num):
 print(x[i][1])
"""
"""
num =1000000
for i in range(num):
 x[i] =i
for i in range(10):
 x[i] = x[i]
 print(x[i])
"""


