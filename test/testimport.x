addpath("../../")
from xlang_cantor import cantor
removepath("../../")
from xlang_os import utils
uid = utils.generate_uid()
print("uid=",uid)
frm = cantor.DataFrame(10)
size = frm.get_Size()
print("size=",size)
cantor.PushFrame(frm)
print("end")
