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

from xlang_image import Factory
im =  Factory.Image("bg.jpg") 
t =  im.to_tensor(Factory.rgba)
new_im =  Factory.Image("bg_new.jpg")
new_im.from_tensor(t)
new_im.save()

t_float = t.astype(tensor.float)
x = t_float*nn.conv2d(3,3,64)*w1
t_p = t.permute([2,0,1])
im2 =  Factory.Image("bg2.jpg")
t2 =  im2.to_tensor(Factory.rgb)
print(t,"\n",t2)
