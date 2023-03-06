from xlang_image import Factory
im =  Factory.Image("bg.jpg")
t =  im.to_tensor(Factory.rgba)
im2 =  Factory.Image("bg2.jpg")
t2 =  im2.to_tensor(Factory.rgb)
print(t,"\n",t2)


