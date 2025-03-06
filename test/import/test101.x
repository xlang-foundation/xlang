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

import help deferred as h
h.prop1 = "prop1 from deferred object"
test0 = h.test0
test0.prop2 ="prop2 from deferred object"
h.load()
h.test01("this is from deferred object")
test0.test02("from test2")
print("hello world")
