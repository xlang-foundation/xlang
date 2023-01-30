kernel_port = 7230
run_param = "-c lrpc_listen(${kernel_port},True)"
print("${run_param}")
run_new_instance(run_param)
import builtin as remote_proc thru 'lrpc:${kernel_port}'

def kernel_main():
	output_list =[]
	def output_redirect(info):
		global output_list
		output_list+=info
	def process(info):
		print(info)
	self.setprimitive("Output",output_redirect)

code = kernel_main.getcode()
print(code)
remote_module = remote_proc.new_module()
remote_module.runfragment(code)
remote_module.process("from client")

