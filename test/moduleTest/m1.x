kernel_port = 7230
run_param = "-c lrpc_listen(${kernel_port},True)"
print("${run_param}")
run_new_instance(run_param)
import bulitin as remote_proc thru 'lrpc:${kernel_port}'

def kernel_main:
	

remote_module = remote_proc.new_module()
remote_module.runfragment()

remote_proc.setprimitive("output",)
