from xlang_barelink import  barelink
devices = barelink.EnumDevices()
print("Device:",devices)

# Connect to the first device
device = barelink.Device("COM4")
device.Connect()
# Load code to the device
device.RunCommand(0,"print('Hello World')")
device.RunCommand(1,"")
print("Connected to device")

