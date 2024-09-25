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

from xlang_barelink import  barelink
from xlang_os import fs


def LoadCode(filename):
  code = "";
  f = fs.File(filename,"r");
  f_size = f.size;
  if f_size >=0:
    code = f.read(f_size)
  f.close();
  return code

# Load code to the device
code = LoadCode("pico_test.x")


devices = barelink.EnumDevices()
print("Device:",devices)

# Connect to the first device
device = barelink.Device("COM4")
device.Connect()
device.RunCommand(0,code)
device.RunCommand(1,"")
print("Connected to device")

