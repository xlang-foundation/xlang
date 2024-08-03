from xlang_barelink import  barelink
from xlang_os import fs


def LoadCode(filename):
  code = "";
  f = fs.File(filename,"r");
  f_size = f.size;
  if f_size >=0:
    code = f.read(f_size)
  f.close();
  return code

# Load code to the device
code = LoadCode("pico_test.x")


devices = barelink.EnumDevices()
print("Device:",devices)

# Connect to the first device
device = barelink.Device("COM3")
device.Connect()
device.RunCommand(0,code)
device.RunCommand(1,"")
print("Connected to device")

