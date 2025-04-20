# udp_client.x
from xlang_net import Net
import time

# Create a UDP client. We can bind to any available local port (e.g., 0 lets the OS choose).
udp = Net.Udp("127.0.0.1", 0)

# Give the server a moment to start.
time.sleep(0.1)

# Send a test packet to the server on port 12345.
message = "Hello from UDP client!"
bin_message = bytes(message, Serialization = True)
udp.sendPacket("127.0.0.1", 12345, bin_message)

print("UDP client sent:", message)
