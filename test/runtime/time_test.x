#
# Copyright (C) 2024 The XLang Foundation
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# <END>

import time
# 1. Get current time in seconds since the Epoch
var1 = time.time()
print("Current time in seconds since the Epoch:", var1)

# 2. Get the local time as a struct_time
var2 = time.localtime(var1)
print("Local time as struct_time:", var2.tm_year, "-", var2.tm_mon, "-", var2.tm_mday, " ", var2.tm_hour, ":", var2.tm_min, ":", var2.tm_sec, " wday:", var2.tm_wday, " yday:", var2.tm_yday, " isdst:", var2.tm_isdst)

# 3. Get the GMT time as a struct_time
var3 = time.gmtime(var1)
print("GMT time as struct_time:", var3.tm_year, "-", var3.tm_mon, "-", var3.tm_mday, " ", var3.tm_hour, ":", var3.tm_min, ":", var3.tm_sec, " wday:", var3.tm_wday, " yday:", var3.tm_yday, " isdst:", var3.tm_isdst)

# 4. Format the local time into a readable string
var4 = time.strftime("%Y-%m-%d %H:%M:%S", var2)
print("Formatted local time:", var4)

# 5. Parse a formatted time string back into a struct_time
time_string = "2024-08-24 15:32:36"
var5 = time.strptime(time_string, "%Y-%m-%d %H:%M:%S")
print("Parsed time string back to struct_time:", var5.tm_year, "-", var5.tm_mon, "-", var5.tm_mday, " ", var5.tm_hour, ":", var5.tm_min, ":", var5.tm_sec, " wday:", var5.tm_wday, " yday:", var5.tm_yday, " isdst:", var5.tm_isdst)

# 6. Sleep for a short duration (0.1 seconds)
time.sleep(0.1)
print("Slept for 0.1 seconds.")

# 7. Get the difference between two times
start_time = time.strptime("2024-08-24 15:00:00", "%Y-%m-%d %H:%M:%S")
end_time = time.strptime("2024-08-24 16:00:00", "%Y-%m-%d %H:%M:%S")
var6 = time.timediff(start_time, end_time)
print("TimeDiff between two times (seconds):", var6)

# 8. Calculate elapsed time from a given start time until now
var7 = time.timeelapsed(start_time)
print("TimeElapsed from a given start time until now (seconds):", var7)

# 9. Add 3600 seconds (1 hour) to a given time
var8 = time.timeadd(start_time, 3600)
print("TimeAdd after adding 3600 seconds:", var8.tm_year, "-", var8.tm_mon, "-", var8.tm_mday, " ", var8.tm_hour, ":", var8.tm_min, ":", var8.tm_sec, " wday:", var8.tm_wday, " yday:", var8.tm_yday, " isdst:", var8.tm_isdst)

print("Done")