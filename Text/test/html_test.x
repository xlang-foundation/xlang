import html
x = html.load("test.html") """xxxxx"""
y = x.query("""
	<query>
		<match>
			<div ${output}='${0}.attr("data-id")'>
				<div style='color:aqua' ${output}='${0}.Content'></div>
			</div>
		</match>
	</query>
""")
print(x)