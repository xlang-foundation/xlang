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

import cantor thru 'lrpc:1000'
df = cantor.DataFrame(1024)
df.type =1000
df.startTime =1234
df.format1 =1
print(df.type)
d = [](size=500,init='rand(1.0,100.0)')
df.data =d
x = df.data
host = cantor.Host()
uid = host.generate_uid()
df.sourceId =uid
print(uid)
host.PushFrame(df)
print("end")
