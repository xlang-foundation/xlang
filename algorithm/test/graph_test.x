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

from xlang_algorithm import algorithm

g =	 algorithm.graph()
g.AddEdge(0, 1, 2);
g.AddEdge(1, 2, 3);
g.AddEdge(2, 3, 5);
g.AddEdge(3, 4, 2);
g.AddEdge(4, 0, 4);
p = g.Prim_MST(0)
print(p)
c = g.PTreeChildren(p,1)
print(c)
sub = g.SubTree(p,1)
sub_tree = sub[0]
sub_map = sub[1]
print("end")

