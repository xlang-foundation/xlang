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

print("*******************************************")
# add a number to a tensor
t1 = tensor([[1,2,3],[4,5,6],[7,8,9],[10,11,12]])
print("t1 =", t1)

t11 = 10 + t1
t_g = T.graph(t11)
print(t_g)
print("before graph run, 10+t1=", t11)
t_g.run()
print("after graph run, 10+t1=", t11)
#expected result
#10+t= Tensor(size=(4,3),[11,12,13,14,15,16,17,18,19,20,21,22])
print("*******************************************")

t12 = t1 + 10
t_g = T.graph(t12)
print(t_g)
print("before graph run, t1+10=", t12)
t_g.run()
print("after graph run, t1+10=", t12)
#expected result
#t1+10= Tensor(size=(4,3),[11,12,13,14,15,16,17,18,19,20,21,22])
print("*******************************************")

t13=t1.Add(10)
t_g = T.graph(t13)
print(t_g)
print("before graph run, t1.Add(10)=", t13)
t_g.run()
print("after graph run, t1.Add(10)=", t13)
#expected result
#t1+10= Tensor(size=(4,3),[11,12,13,14,15,16,17,18,19,20,21,22])
print("*******************************************")

#to add a tensor to a tensor
t1 = tensor([[1,2,3],[4,5,6],[7,8,9],[10,11,12]])
print("t1 =", t1)
t2 = tensor([[10,20,30],[40,50,60],[70,80,90], [100,110,120]])
print("t2 =", t2)
t15 = t1+t2
print("before graph run, t1+t2=", t15)
t_g = T.graph(t15)
print(t_g)
t_g.run()
print("after graph run, t1+t2=", t15)
#expected result
#t1+t2= Tensor(size=(4,3),[11,22,33,44,55,66,77,88,99,110,121,132])
print("*******************************************")


