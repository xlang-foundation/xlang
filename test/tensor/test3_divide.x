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

import CpuTensor as T
t1 = tensor([[1,2,3],[4,5,6],[7,8,9]])
t2 = tensor([[10,20,30],[40,50,60],[70,80,90]])

print("t1 =", t1)
print("t2 =", t2)
print ("***************************************************************")

#to divide by a number
t3 = t2/2
#t1 =t1*10
#t1 = 2*t1
t_g = T.graph(t3)
t_g.run()
print("t2/2=", t3)
#expected result 
#Tensor(size=(3,3),[5,10,15,20,25,30,35,40,45])
print ("***************************************************************")

t3 = 90/t1
#t1 =t1*10
#t1 = 2*t1
t_g = T.graph(t3)
t_g.run()
print("90/t1=", t3)
#expected result 
#Tensor(size=(3,3),[90,45,30,22,18,15,12,11,10])
print ("***************************************************************")

#to divide by a number
t2/=2
#t1 =t1*10
#t1 = 2*t1
t_g = T.graph(t2)
t_g.run()
print("t2/2=", t2)
#expected result 
#Tensor(size=(3,3),[20,40,60,80,100,120,140,160,180])
print ("***************************************************************")

#to divide a tensor by a tensor
t3 = t2 / t1
#t1*t2
#t1.MUL(t2)
print(t3)
t_g = T.graph(t3)
t_g.run()
print("t2/t1=", t3)
#expected result 
#Tensor(size=(3,3),[10,40,90,160,250,360,490,640,810])
print ("***************************************************************")


