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

#to minus a tensor to a tensor with different dimensions
t1 = tensor([[[1,2],[3,4]],[[5,6],[7,8]],[[9,10],[11,12]]])
print("t1 =", t1)
t2 = tensor([[1,2],[3,4]])
print("t2 =", t2)
import CpuTensor as T
t11 = t1-t2
print("before graph run, t1-t2=", t11)
t_g = T.graph(t11)
print(t_g)
t_g.run()
print("after graph run, t1-t2=", t11)

#expected t1-t2= Tensor(size=(3,2,2),[0,0,0,0,4,4,4,4,8,8,8,8])
