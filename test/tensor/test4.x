# Test image from tensor

from xlang_image import Factory
im =  Factory.Image("test_rand.jpg")
t0 = tensor.randwithshape([1080,1920,4],min=0,max=255,dtype=tensor.int8)
im.from_tensor(t0)
im.save()
print("end")

