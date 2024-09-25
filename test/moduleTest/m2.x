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

kernel_port = 7230
run_param = "-event_loop -c lrpc_listen(${kernel_port},False)"
print("${run_param}")
run_new_instance(run_param)
import builtin as remote_proc thru 'lrpc:${kernel_port}'

def kernel_main():
	output_list =[]
	kernel_module = None
	def output_redirect(info):
		global output_list
		output_list+=info
	def process(code):
		kernel_module.runfragment(code)
	def get_outputs():
		global output_list
		l = tostring(output_list)
		output_list =[]
		return l
	kernel_module = new_module()
	kernel_module.setprimitive("Output",output_redirect)

code = kernel_main.getcode()
print(code)
kernel_module = remote_proc.new_module()

module_key = remote_proc.loads(code)
r_m = remote_proc.get_modulebykey(module_key)
remote_proc.mainrun(module_key)
r_m.process("test from remote")
l = r_m.get_outputs()
print("end,${l}")
