addpath("../../")
from xlang_cantor import cantor
removepath("../../")
kv = cantor.KV()
v = kv.Get(7)
print("before set,v =",v,"\n")
kv.Set(7,"new key:7")
v = kv.Get(7)
print("end,v=",v,"\n")
