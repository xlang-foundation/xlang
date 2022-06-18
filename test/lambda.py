f = (x:int,y:int,z:int){
  f2 =(k:int){
       print("      inside lambda f2:",k);
       return k;
    }
    r = f2(x);
    r2 =f2(x+1);
    print("  xyz:",x,y,z,",r=",r,",r2=",r2);
}

def Test(f0):
    print("call lambda")
    f0(10,2,3)
    print("end call")

Test(f)
