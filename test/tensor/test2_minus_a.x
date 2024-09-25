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
#a number minus a tensor
t1 = tensor([[1,2,3],[4,5,6],[7,8,9],[10,11,12]])
print("t1 =", t1)
t11 = 10 - t1

t_g = T.graph(t11)
print(t_g)
print("before graph run, 10 - t1=", t11)
t_g.run()
print("after graph run, 10 - t1=", t11)
#expected result
#t1-10= Tensor(size=(4,3),[-9,-8,-7,-6,-5,-4,-3,-2,-1,0,1,2])

print("*******************************************")

