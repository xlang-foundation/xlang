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

for i in range(2):
	print("Table-----${i}------")
	%<H1>Pipeline</H1>
	%<Table>
	for k in range(2):
		%<tr>
		for j in range(3):
			%	<td>${j}</td>
			%	<td>
			%		<img src="bg_k_${k}.jpg" data-i=${k}/>
			%	</td>
		%</tr>
	%</Table>
f_size =1
f123 =(){
	print("enter func");
	if f_size>=0:
		print(">=0");
	else:
		print("else");
	print("out func");
}
f123()
print("End")