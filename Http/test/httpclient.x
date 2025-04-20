﻿#
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

from xlang_http import http
client = http.Client("https://www.oxfordlearnersdictionaries.com")
client.post("/us/definition/english/transcend?q=transcending")
s = client.body
print("fetching done:",s)

client.get("/us/definition/english/enormously?q=enormously")
s2 = client.body
print("fetching done-2:",s2	)



# Define the portal server URL (using Postman Echo for testing)
portal_server_url = "https://postman-echo.com"

# Create an HTTP client instance with the base URL
http_client2 = http.Client(portal_server_url)

body = '{"username": "johndoe", "email": "john@example.com", "action": "verify"}'

http_client2.post("/post", "application/json", body)

print("Response Body:", http_client2.body)
print("Done")
