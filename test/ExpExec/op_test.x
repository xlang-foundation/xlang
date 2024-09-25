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

i = 200
if i in [10,20,30]:
	print(i)
else:
	print("not in")
if i in range(10):
	print("i=",i)
pos =0

while pos in range(10):
	print("pos=",pos)
	pos+=1
for (i,pos) in range(10):
	print("i=",i)
	if i > 3:
		break
	if i == 2:
		continue
	print("after if",i)
print("end")

for (i,pos) in [1,3,5,7,9,11]:
	print(i)
print("end 2")