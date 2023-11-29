
import os
def print_func(info):
    print("from python function:",info)
    pid = os.getpid()
    return pid

def call_xlang_func(xfunc):
    xfunc("from python")
    print("after call xfunc")
