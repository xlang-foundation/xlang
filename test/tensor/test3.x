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

t0 = tensor.randwithshape([10,3,64,64],min=-1,max=1)
t2 = t0[0,0,-2:2,63:70]
t3 = t2[3,73]
t4 = t2[3,4]
t5 = t2[100,100]
print("t2=${t2}")
