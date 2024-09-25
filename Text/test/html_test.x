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

import html
x = html.load("test.html")
y = x.query("""
	<query>
		<match>
			<div ${output}='${0}.attr("data-id")'>
				<div style='color:aqua' ${output}='${0}.Content'></div>
				<a href='test'></a>
			</div>
		</match>
	</query>
""")
if y:
	k = y[0].attrs["data-id"]
	z = y[0].kids[0].content
print(x)