i = 200
if i in [10,20,30]:
	print(i)
else:
	print("not in")
if i in range(10):
	print("i=",i)
pos =0

while pos in range(10):
	print("pos=",pos)
	pos+=1
for (i,pos) in range(10):
	print("i=",i)
	if i > 3:
		break
	if i == 2:
		continue
	print("after if",i)
print("end")

for (i,pos) in [1,3,5,7,9,11]:
	print(i)
print("end 2")