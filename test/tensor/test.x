from xlang_image import Factory
im =  Factory.Image("bg.jpg")
t =  im.to_tensor(Factory.rgba)
t_float = t.astype(tensor.float)
t_p = t.permute([2,0,1])
im2 =  Factory.Image("bg2.jpg")
t2 =  im2.to_tensor(Factory.rgb)
print(t,"\n",t2)


