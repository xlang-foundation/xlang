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

var1 =100
def test():
	print("inside test,var1:",var1)

def test2(x,y):
	print("inside test2:",x,",",y)

class test_class01():
    prop0:str="prop0_value"
    def test(x,y):
        print("from test_class01's test():",x,",",y,",prop0=",this.prop0)
