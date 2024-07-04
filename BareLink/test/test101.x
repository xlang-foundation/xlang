from xlang_barelink import  barelink
devices = barelink.EnumDevices()
print("Device:",devices)

# Connect to the first device
device = barelink.Device("COM4")
device.Connect()
print("Connected to device")

