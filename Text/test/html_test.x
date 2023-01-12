import html
x = html.load("test.html")
y = x.query("""
	<query>
		<match>
			<div ${output}='${0}.attr("data-id")'>
				<div style='color:aqua' ${output}='${0}.Content'></div>
				<a href='test'></a>
			</div>
		</match>
	</query>
""")
if y:
	k = y[0].attrs["data-id"]
	z = y[0].kids[0].content
print(x)