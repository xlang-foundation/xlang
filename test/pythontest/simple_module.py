
import os
def print_func(info):
    print("from python function:",info)
    pid = os.getpid()
    return pid
