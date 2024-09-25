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

print("taskrun test")
pool = taskpool(max_task_num=10,run_in_ui=False)
def test_fn(x,y):
	sleep(10000)
	return x+y

def get_result(r):
	f = (){
		print("mainrun,get_result:",r,"tid=",threadid());
	}
	mainrun(f)
	print("get_result:",r,"tid=",threadid())


f = test_fn.taskrun(pool,10,20).then((r){
	print("inside Then call:r=",r,"tid=",threadid());
}).then((r){
	print("Second *** inside Then call:r=",r,"tid=",threadid());	
}
)
f.then(get_result)
k  = f.get(100)
print("get k:",k,"tid=",threadid())
x = input()
print("end")
