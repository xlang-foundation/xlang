import html
x = html.load("test.html")
y = x.query("
	<query>
		<match>
			<div>
				<div style='color:aqua'></div>
			</div>
		</match>
	</query>
")
print(x)