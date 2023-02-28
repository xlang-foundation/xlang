kernel_port = 7230
run_param = "-event_loop -c lrpc_listen(${kernel_port},False)"
print("${run_param}")
run_new_instance(run_param)
import builtin as remote_proc thru 'lrpc:${kernel_port}'

def kernel_main():
	output_list =[]
	fragModule = new_module()
	def output_redirect(info):
		global output_list
		output_list+=info
	def process(code):
		fragModule.runfragment(code)
	def get_outputs():
		global output_list
		l = tostring(output_list)
		output_list =[]
		return l
	fragModule.setprimitive("Output",output_redirect)

code = kernel_main.getcode()
#print(code)
remote_module = remote_proc.new_module()
remote_module.runfragment(code)


p =pid()
print("this is from pid:${p}")
code ="
p =pid()
print('from interactive module,pid=',p)
"
remote_module.process(code)

code2 ="
p =pid()
print('from interactive module,code 2,pid=',p)
"
remote_module.process(code2)
l = remote_module.get_outputs()
print("list:${l}")
code3 ="
p =pid()
print('from interactive module,code 3,pid=',p)
"
remote_module.process(code3)
l = remote_module.get_outputs()
print("list:${l}")
