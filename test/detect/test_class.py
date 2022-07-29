class test_class():
    prop0 = "prop0_value"
    def __init__(self):
        print("__init__")
    def test(self,x,y):
        print("from base_class's test():",x,y)
def func1(x,y):
    print(x,y)