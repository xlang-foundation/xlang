# udp_srv.x
from xlang_net import Net

# Define the callback function that will be called when a packet is received.
def on_receive(packet,send_ip,port):
    msg = fromBytes(packet)
    print("Server received:", msg)

# Create a UDP server bound to all interfaces on port 12345.
udp = Net.Udp("0.0.0.0", 12345)
# Set the receive callback.
udp.setReceiveCallback(on_receive)

print("UDP server is running on port 12345...")

x = input("Press Enter to stop the server...")
# Stop the server.

