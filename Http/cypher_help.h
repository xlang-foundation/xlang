#ifndef RSA_ENCRYPTION_H
#define RSA_ENCRYPTION_H

#include <vector>
#include <string>
#include <openssl/rsa.h>

std::vector<unsigned char> long_msg_encrypt_with_private_key(const std::string& message, RSA* rsa);
std::string long_msg_decrypt_with_public_key(const std::vector<unsigned char>& encrypted, RSA* rsa);
std::vector<unsigned char> long_msg_encrypt_with_public_key(const std::string& message, RSA* rsa);
std::string long_msg_decrypt_with_private_key(const std::vector<unsigned char>& encrypted, RSA* rsa);

#endif // RSA_ENCRYPTION_H
