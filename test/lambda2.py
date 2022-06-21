def Test(f0):
    return f0(10,2,3)
r = Test((x,y,z){
             for i in range(10):
                print("xyz=",i,x,y,z)
             print("loop,",i)
             return x+y+z;
      })
print("r=",r)
