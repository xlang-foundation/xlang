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
#t1 = tensor([[1,2,5],[0,0,0], [1,2,1]])
t1 = tensor([[1,2,5],[7, 5, 9], [4,2,1]])
print("t1 =", t1)
#t2 = tensor([[-1,-2,-1],[0,0,0], [1,2,1]])
t2 = tensor([[1,2,3], [4,5,6], [7,8,9]])
print("t2 =", t2)
print ("***********************************************************************")
t11 = t1*T.conv2d()*t2
t_g = T.graph(t11)
t_g.run()
print("after graph run, t1*T.conv2d()*t2=", t11)
#expected sum is = [[-16, -24,-28,-23],[-24,-32,-32,-24],[-24,-32,-32,-24],[28,40,44,35]]  //same mode
#expected result t1*T.conv2d()*t2= Tensor(size=(6,6),[-1,-4,-8,-12,-11,-4,-5,-16,-24,-28,-23,-8,-8,-24,-32,-32,-24,-8,-8,-24,-32,-32,-24,-8,9,28,40,44,35,12,13,40,56,60,47,16])


