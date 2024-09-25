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

def test_return(x):
	if x == 1:
		x +=100
		return x
	elif x == 2:
		return 2
	elif x == 3:
		return 3
	print("This is last code")
	return 10

y = test_return(1)
print(y)
y = test_return(9)
print(y)
