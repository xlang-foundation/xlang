import xlang
import time
import threading

http = xlang.importModule("http", fromPath="xlang_http")

# Define the event handler
def on_test_event(event_data):
    print(f"Received event in Python! Data: {event_data}")

# Register the event handler
http.TestEvent += on_test_event

# Fire the event from multiple C++ threads
print("Firing events from C++...")
http.FireTestEvent()

# Wait for a bit to allow threads to complete
time.sleep(2)
print("Test completed.")
