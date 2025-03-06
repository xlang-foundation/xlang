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

x100=100
y100=101

def f1(v):
   x0 =v
   x1 =x0+1
   if v==1:
      f1(100)
   
   def f2():
      print("x0=",x0)
   f2()
   

f1(1)



import datetime

x =1

start_time = datetime.datetime.now()

def func1(x,y):
   print("func1")
f =func1
f(1,1)
x = 0
num  =10000000
for i in range(num):
   x += i
   x = x+3
   x = x-3

end_time = datetime.datetime.now()

time_diff = (end_time - start_time)
execution_time = time_diff.total_seconds() * 1000

print("num=",num,execution_time,",sum=",x)
print("end")
