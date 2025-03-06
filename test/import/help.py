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

prop1 =100

def test01(info):
    print(info,",prop1=",prop1)
    return info

class test():
    def __init__(self):
        self.prop2 = 200

    def test02(self,info):
        print(info,",prop1=",prop1,",prop2=",self.prop2)
        return info
    
test0= test()
