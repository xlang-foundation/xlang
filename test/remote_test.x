import cantor thru 'lrpc:1000'
kv = cantor.KV()
net  = cantor.Network()
net.StartServer(3142,10)
v = kv.Get(7)
print("before set,v =",v,"\n")
kv.Set(7,"new key:7")
v = kv.Get(7)
print("end,v=",v,"\n")
