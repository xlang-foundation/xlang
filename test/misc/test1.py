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

def func1(i0:int,i1:int):
	x =i0
	y=i1
	z=x+y
	return z
num =10
def taskfunc(n):
	var1 =0
	for i in range(num):
		var1 =var1+ func1(i,100)
		print("tid:",threadid(),",sum:",var1)
	return var1
t = time()
print("main tid:",threadid())
taskfunc.taskrun(num)
taskfunc(num)
t2 = time()
print("timespend=",t2-t)
sleep(10000)
