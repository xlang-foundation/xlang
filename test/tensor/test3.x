t0 = tensor.randwithshape([10,3,64,64],min=-1,max=1)
t2 = t0[0,0,-2:2,63:70]
t3 = t2[3,73]
t4 = t2[3,4]
t5 = t2[100,100]
print("t2=${t2}")
