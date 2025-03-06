from "sub" import func_from_sub as sub

x = sub.SubFunc()
print("call from sub folder's module, x=${x}")

from "../misc" import simple
y = simple.test_return(100)
print("y=${y}")
print("Done")
