x =<|Name:string:nullable='shawn',Age:int64,Weight:double|>(
    "shawn01",10,134.4,
    "shawn02",20,122.33,
    "Daniel",15,145.12
    )
for i in range(10000):
    x+=["shawn"+i,i+100,19.2+i]
#x.filter(Name == 'Danel' && Age>10).select(Name,Age)
print(x)