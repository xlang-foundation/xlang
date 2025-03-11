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

import time
num =1000000
print("Test Loop with XLang Jit:",num)
@jit
def sum_all(toNum:unsigned long long)->unsigned long long:
    unsigned long long sum = 1;
    for(unsigned long long i=0;i<toNum;i++)
    {
        sum += i;
    }
    return sum;
print("start")
@jit
def test_second(m:int,n:int)->int:
    return m+n;

t1 = time.time()
sum = sum_all(num)
t2 = time.time()
t2 = (t2-t1)
print("sum=",sum)
k = test_second(1,2)
print("k=",k)
print("All->time spend:",t2)

print("end")
