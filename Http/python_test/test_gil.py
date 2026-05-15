import xlang
import time
import sys
import threading
import psutil
import os

http = xlang.importModule("http", fromPath="xlang_http")

# We expect 10 threads * 10,000 events = 100,000 events
expected_events = 100000
received_events = 0

import gc

def get_memory_mb():
    process = psutil.Process(os.getpid())
    return process.memory_info().rss / (1024 * 1024)

def on_test_event(event_data):
    global received_events
    received_events += 1
    if received_events % 10000 == 0:
        gc.collect()
        print(f"Received {received_events} events so far... Memory: {get_memory_mb():.2f} MB")

# Register the event handler
http.TestEvent += on_test_event

print(f"Starting pressure test... Firing {expected_events} events from C++...")
print(f"Initial Memory: {get_memory_mb():.2f} MB")
# Fire the event from multiple C++ threads
start_time = time.time()
http.FireTestEvent()
end_time = time.time()

print(f"Pressure test completed in {end_time - start_time:.2f} seconds.")
print(f"Total events received: {received_events}")

if received_events != expected_events:
    print("FAILED: Did not receive all events!")
    sys.exit(1)
else:
    print("SUCCESS: All events received without crashing!")
    sys.exit(0)

