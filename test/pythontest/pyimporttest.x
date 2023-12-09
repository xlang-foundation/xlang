import os
x = os.getcwd()
x0 = to_xlang(x)
import simple_module
pid = simple_module.print_func("check into python function")
x_pid = to_xlang(pid)
print("end run")