
def Output(info):
	print("Output Primitive:",info)
m = new_module()
m.setprimitive("Output",Output)
m.runfragment("""
	x=1
	print("x=${x}")
	""")
print("test")
