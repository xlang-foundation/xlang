x ={
   "key1":(x,y){
       print("key1--",x,y);
       for i in range(10):
        print(" again key1--",i,x+y);
        print(" again(2) key1--",i,x+y);
       return x*y;
   },
   "2":(){
       print("key---2");
   },
   2:(x,y,z){
       print("number key 2,",x,y,z);
   }
}
k = x["key1"](20,22)
print("k=",k)
x["2"]()
x[2](19,72,23)



