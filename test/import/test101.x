import help deferred as h
h.prop1 = "prop1 from deferred object"
test0 = h.test0
test0.prop2 ="prop2 from deferred object"
h.load()
h.test01("this is from deferred object")
test0.test02("from test2")
print("hello world")
