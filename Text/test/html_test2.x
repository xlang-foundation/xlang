import html
x = html.load("test.html")
y = x.query("
	<query>
		<match>
			<filter>
				<div ${child_combinator}='direct|any'>
					<div style='color:aqua'></div>
				</div>
				<div ${sibling_combinator}='adjacent|any' ${logical}='and|or'>
					<div style='color:aqua'></div>
				</div>
			</filter>
			<output>
				<result>
					${0}.Content
				</result>
			</output>
		</match>
	</query>
")
print(x)