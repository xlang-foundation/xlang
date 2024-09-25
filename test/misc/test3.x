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

m1 ="100"
m2 = 1992

m1 +=10

def test(x1,x2):
  extern m1,m2
  print("inside test m1=",m1,",m2=",m2)
  m1 = x1+x2
  m2 += x2
print("before test func called,m1=",m1,",m2=",m2)
test(11,1000)
print("after test func called,m1=",m1,",m2=",m2)


