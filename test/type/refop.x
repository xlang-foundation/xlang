#
# Copyright (C) 2024 The XLang Foundation
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# <END>

var1 = 10
v1 = ref var1
v1_r = v1()
t = ref(var1+10)
t1 = t()


def test_ref(p1,p2):
    r1 = ref p1
    r2 = ref p2
    all = r1()+r2()
    return all

test1 = test_ref("This is ","a nice test for ref")
test2 = test_ref(3.14,1000)

def test2(p)
    r = p()
    return r
test2(ref(test_ref(1,2)+10000))
print("Done")