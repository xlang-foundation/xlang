kernel_port = 7230
run_param = "-event_loop -c lrpc_listen(${kernel_port},False)"
print("${run_param}")
run_new_instance(run_param)
import builtin as remote_proc thru 'lrpc:${kernel_port}'

def kernel_main():
	_lock =[]
	output_list =[]
	kernel_module = None
	def output_redirect(info):
		global output_list
		_lock.lock()
		output_list+=info
		_lock.unlock()
	def process(code):
		fire("run_code",kernel_module,code,mainthread = True)
	def get_outputs():
		global output_list
		_lock.lock()
		l = tostring(output_list)
		output_list.clear()
		_lock.unlock()		
		return l
	on("run_code",(m,code){
		m.runfragment(code);
	})
	kernel_module = new_module()
	kernel_module.setprimitive("Output",output_redirect)

code = kernel_main.getcode()
print(code)
r_m = remote_proc.new_module()
r_m.runfragment(code)

def test_code():
	sleep(time=1000)
	p1 = pid()
	for i in range(10000):
		sleep(time=1)
		print('from client 1,i=${i},pid=${p1}')
	print("end loop")
test_c = test_code.getcode()
print(test_c)
r_m.process(test_c)
for i in range(10000):
	l = r_m.get_outputs()
	print("i:${i},${l}")
	sleep(time=1)
r_m.process("print('from client 2')")
l = r_m.get_outputs()
print("end,${l}")
