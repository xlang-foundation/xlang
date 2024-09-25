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
from Galaxy import galaxy

galaxy.cantor = cantor

class Run_Sam_filter(galaxy.BaseFilter):
	inputPin:Pin = None
	outputPin:Pin = None
	file_path ="c:\\"
	version = 1.0
	def Run_Sam_filter():
		this.inputPin = NewInputPin()
		this.outputPin = NewOutputPin()
		SetRunFunc(this.Run)
	def Run():
		print("in Run")

runSamfilter = Run_Sam_filter()

cantor_type = type(cantor)
cantor_members = cantor_type.getMembers()

galaxy_type = type(galaxy)
galaxy_members = galaxy_type.getMembers()

runSamfilter_type = type(runSamfilter)
runSamfilter_members = runSamfilter_type.getMembers()


print("end")

