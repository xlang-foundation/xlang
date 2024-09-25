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

x =<|Name:string:nullable='shawn',Age:int64,Weight:double|>(
    "shawn01",10,134.4,
    "shawn02",20,122.33,
    "Daniel",15,145.12
    )
for i in range(10000):
    x+=["shawn"+i,i+100,19.2+i]

for i in range(10000):
    x+=["Jack"+i,i+100,19.2+i]

y = x.filter(Name == 'Danel' and Age>10).select(Name+"_name",Age-10)
z =  y.join(x,y.col1==x.col2).filter(Name in ['dd'])

print(z)