prop1 =100

def test01(info):
    print(info,",prop1=",prop1)
    return info

class test():
    def __init__(self):
        self.prop2 = 200

    def test02(self,info):
        print(info,",prop1=",prop1,",prop2=",self.prop2)
        return info
    
test0= test()
