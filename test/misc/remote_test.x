import cantor thru 'lrpc:1000'
df = cantor.DataFrame(1024)
df.type =1000
df.startTime =1234
df.format1 =1
print(df.type)
d = [](size=500,init='rand(1.0,100.0)')
df.data =d
x = df.data
host = cantor.Host()
uid = host.generate_uid()
df.sourceId =uid
print(uid)
host.PushFrame(df)
print("end")
