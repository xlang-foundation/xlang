
def Output(info):
	print("Output Primitive:",info)
m = new_module()
m.setprimitive("Output",Output)
print("OK")
m.runfragment("""
    class base_class():
        prop0:str="prop0_value"
        def test(x,y):
            print("from base_class's test():",x,y)
    b = base_class()
    b.test(10,20)
	""")
print("test")
