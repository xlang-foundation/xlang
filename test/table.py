x =<|Name:string:nullable='shawn',Age:int64,Weight:double|>(
    "shawn01",10,134.4,
    "shawn02",20,122.33,
    "Daniel",15,145.12
    )
for i in range(10000):
    x+=["shawn"+i,i+100,19.2+i]

for i in range(10000):
    x+=["Jack"+i,i+100,19.2+i]

y = x.filter(Name == 'Danel' and Age>10).select(Name+"_name",Age-10)
z =  y.join(x,y.col1==x.col2).filter(Name in ['dd'])

print(z)