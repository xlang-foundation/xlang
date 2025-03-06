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

print("taskrun simple test")
pool = taskpool(max_task_num=1,run_in_ui=False)
def test_fn(x,y):
	sleep(1000)
	print("test_fn,x:${x},y:${y}","，tid:",threadid())
	return x+y
print("start dispatch task...")
f = test_fn.taskrun(pool,10,20)
f1 = test_fn.taskrun(pool,20,40)
f1.cancel()
f.then((v){
	print("Total time:",f.getTotalTime());
	print("Run time:",f.getRunTime());
	print("Return:",v);
})
f2 = test_fn.taskrun(pool,30,80)
f3 = test_fn.taskrun(pool,50,100)
f_all =[]
for i in range(10):
	f_all+=test_fn.taskrun(pool,100+i*10,100)
f_ret = f.get(10000)
print("end task dispatch,input somethong to end:")
x = input()
print("end")
