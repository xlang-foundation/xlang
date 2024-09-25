/*
Copyright (C) 2024 The XLang Foundation
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef RSA_ENCRYPTION_H
#define RSA_ENCRYPTION_H

#include <vector>
#include <string>
#include <openssl/rsa.h>

std::vector<unsigned char> long_msg_encrypt_with_private_key(int paddingMode,const std::string& message, RSA* rsa);
std::string long_msg_decrypt_with_public_key(int paddingMode, const std::vector<unsigned char>& encrypted, RSA* rsa);
std::vector<unsigned char> long_msg_encrypt_with_public_key(int paddingMode, const std::string& message, RSA* rsa);
std::string long_msg_decrypt_with_private_key(int paddingMode, const std::vector<unsigned char>& encrypted, RSA* rsa);

#endif // RSA_ENCRYPTION_H
