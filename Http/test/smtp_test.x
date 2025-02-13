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

from xlang_http import smtp

smtp.cert_path = "d:/cacert.pem"
smtp.client_id = ""
smtp.client_secret = ""
smtp.tenant_id = ""
smtp.smtp_scope = "https://outlook.office365.com/.default"
smtp.smtp_server = "cantorai-com.mail.protection.outlook.com"
smtp.smtp_port = 25

send_from = "wtk@cantorai.com"
send_to = "wt_2k@msn.com"
sub = "test send from email sender"
content = "test content"

ret = smtp.send(send_from, send_to, sub, content)

print(ret)

print("========================================================")