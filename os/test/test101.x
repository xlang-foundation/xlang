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

from xlang_os import os
process = os.Process()
process.ProgName = "powershell"
process.Parameter = '-Command "Get-ChildItem D:/test"'
retInfo = process.Run(True)


from xlang_os import fs

folder = fs.Folder("D:/test/")
newPath = folder.BuildPath("new01")
new01 = fs.Folder(newPath)
all = folder.Scan()
simple = folder.List()
folder_1 = fs.Folder("D:/test/set01")
folder.CreateFolder("D:/test/new01")
folder_1.CopyFolder("D:/test/new01")
folder.RemoveFolder("D:/test/new01")
print("done")

