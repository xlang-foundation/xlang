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
x = html.load("test2.html")
y = x.query("
	<query>
		<match>
			<filter>
				<div ${child_combinator}='direct|any' ${node_index}='0-Last'>
					<div style='color:aqua' ${output}="${0}.Content"></div>
				</div>
				<div ${sibling_combinator}='adjacent|any' ${logical}='and|or'>
					<div style='color:aqua'></div>
				</div>
			</filter>
			<output>
				<result>
					${0}.Content
				</result>
			</output>
		</match>
	</query>
")
print(x)