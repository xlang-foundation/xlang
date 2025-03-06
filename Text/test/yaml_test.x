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

import yaml

y = "
---
name: 'this is a name'
port: 3145
search_path: 
 - path1
 - path2
"

k = yaml.loads(y)
c0 =k['search_path',0]
c0_1 =k["search_path"][0]
c =k['search_path',1]
c2 = k["key"]
print("end")
