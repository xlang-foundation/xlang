for i in range(2):
	print("Table-----${i}------")
	%<H1>Pipeline</H1>
	%<Table>
	for k in range(2):
		%<tr>
		for j in range(3):
			%	<td>${j}</td>
			%	<td>
			%		<img src="bg_k_${k}.jpg" data-i=${k}/>
			%	</td>
		%</tr>
	%</Table>
f_size =1
f123 =(){
	print("enter func");
	if f_size>=0:
		print(">=0");
	else:
		print("else");
	print("out func");
}
f123()
print("End")