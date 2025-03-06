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

#include <vector>
#include <string>
#include <stdexcept>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <iostream>
#include <algorithm> // for std::min

// Helper function to calculate the maximum plaintext size per RSA operation,
// taking into account the overhead of the specified padding mode.
size_t get_max_message_size(int paddingMode, RSA* rsa) {
    int rsa_size = RSA_size(rsa);
    switch (paddingMode) {
    case RSA_PKCS1_PADDING:
        if (rsa_size <= 11)
            throw std::runtime_error("RSA key too small for PKCS#1 padding.");
        return rsa_size - 11;
    case RSA_PKCS1_OAEP_PADDING:
        if (rsa_size <= 42)
            throw std::runtime_error("RSA key too small for OAEP padding.");
        return rsa_size - 42;
    case RSA_NO_PADDING:
        // With no padding, the entire RSA block is used.
        return rsa_size;
    default:
        throw std::runtime_error("Unsupported padding mode.");
    }
}

// Function to encrypt a long message using a private key.
// The message is split into chunks of up to get_max_message_size() bytes.
std::vector<unsigned char> long_msg_encrypt_with_private_key(int paddingMode,
    const std::string& message, RSA* rsa) {
    size_t rsa_size = RSA_size(rsa);
    size_t block_size = get_max_message_size(paddingMode, rsa);

    // For RSA_NO_PADDING, the message length must be an exact multiple of rsa_size.
    if (paddingMode == RSA_NO_PADDING && (message.size() % rsa_size != 0)) {
        throw std::runtime_error("Message size must be a multiple of RSA key size when using no padding.");
    }

    std::vector<unsigned char> encrypted;
    // Reserve an estimated capacity.
    encrypted.reserve(rsa_size * ((message.size() / block_size) + 1));

    for (size_t i = 0; i < message.size(); i += block_size) {
        std::vector<unsigned char> buffer(rsa_size);
        size_t chunk_size = std::min(block_size, message.size() - i);
        int encrypted_length = RSA_private_encrypt(
            static_cast<int>(chunk_size),
            reinterpret_cast<const unsigned char*>(message.data() + i),
            buffer.data(),
            rsa,
            paddingMode
        );

        if (encrypted_length == -1) {
            ERR_print_errors_fp(stderr);
            throw std::runtime_error("Encryption failed (private key).");
        }

        encrypted.insert(encrypted.end(), buffer.begin(), buffer.begin() + encrypted_length);
    }

    return encrypted;
}

// Function to decrypt a long message using a public key.
// The encrypted data is assumed to be composed of consecutive RSA blocks.
std::string long_msg_decrypt_with_public_key(int paddingMode,
    const std::vector<unsigned char>& encrypted, RSA* rsa) {
    size_t rsa_size = RSA_size(rsa);
    std::string decrypted;
    decrypted.reserve(encrypted.size());
    size_t maxMessageSize = get_max_message_size(paddingMode, rsa);

    for (size_t i = 0; i < encrypted.size(); i += rsa_size) {
        std::vector<unsigned char> buffer(rsa_size);
        int decrypted_length = RSA_public_decrypt(
            rsa_size,
            encrypted.data() + i,
            buffer.data(),
            rsa,
            paddingMode
        );

        if (decrypted_length == -1) {
            ERR_print_errors_fp(stderr);
            throw std::runtime_error("Decryption failed (public key).");
        }

        // Optional: verify that the decrypted chunk is within expected bounds.
        if (paddingMode != RSA_NO_PADDING && static_cast<size_t>(decrypted_length) > maxMessageSize) {
            throw std::runtime_error("Decrypted chunk exceeds maximum allowed size.");
        }

        decrypted.append(reinterpret_cast<char*>(buffer.data()), decrypted_length);
    }

    return decrypted;
}

// Function to encrypt a long message using a public key.
std::vector<unsigned char> long_msg_encrypt_with_public_key(int paddingMode,
    const std::string& message, RSA* rsa) {
    size_t rsa_size = RSA_size(rsa);
    size_t block_size = get_max_message_size(paddingMode, rsa);

    // For RSA_NO_PADDING, the message length must be an exact multiple of rsa_size.
    if (paddingMode == RSA_NO_PADDING && (message.size() % rsa_size != 0)) {
        throw std::runtime_error("Message size must be a multiple of RSA key size when using no padding.");
    }

    std::vector<unsigned char> encrypted;
    encrypted.reserve(rsa_size * ((message.size() / block_size) + 1));

    for (size_t i = 0; i < message.size(); i += block_size) {
        std::vector<unsigned char> buffer(rsa_size);
        size_t chunk_size = std::min(block_size, message.size() - i);
        int encrypted_length = RSA_public_encrypt(
            static_cast<int>(chunk_size),
            reinterpret_cast<const unsigned char*>(message.data() + i),
            buffer.data(),
            rsa,
            paddingMode
        );

        if (encrypted_length == -1) {
            ERR_print_errors_fp(stderr);
            throw std::runtime_error("Encryption failed (public key).");
        }

        encrypted.insert(encrypted.end(), buffer.begin(), buffer.begin() + encrypted_length);
    }

    return encrypted;
}

// Function to decrypt a long message using a private key.
std::string long_msg_decrypt_with_private_key(int paddingMode,
    const std::vector<unsigned char>& encrypted, RSA* rsa) {
    size_t rsa_size = RSA_size(rsa);
    std::string decrypted;
    decrypted.reserve(encrypted.size());
    size_t maxMessageSize = get_max_message_size(paddingMode, rsa);

    for (size_t i = 0; i < encrypted.size(); i += rsa_size) {
        std::vector<unsigned char> buffer(rsa_size);
        int decrypted_length = RSA_private_decrypt(
            rsa_size,
            encrypted.data() + i,
            buffer.data(),
            rsa,
            paddingMode
        );

        if (decrypted_length == -1) {
            ERR_print_errors_fp(stderr);
            throw std::runtime_error("Decryption failed (private key).");
        }

        if (paddingMode != RSA_NO_PADDING && static_cast<size_t>(decrypted_length) > maxMessageSize) {
            throw std::runtime_error("Decrypted chunk exceeds maximum allowed size.");
        }

        decrypted.append(reinterpret_cast<char*>(buffer.data()), decrypted_length);
    }

    return decrypted;
}

int long_msg_test() {
    // Example usage:
    RSA* rsa = RSA_generate_key(2048, RSA_F4, nullptr, nullptr);

    std::string message = "Hello, this is a test message!";
    // Encrypt using private key and decrypt using public key with OAEP padding.
    std::vector<unsigned char> encrypted = long_msg_encrypt_with_private_key(RSA_PKCS1_OAEP_PADDING, message, rsa);
    std::string decrypted = long_msg_decrypt_with_public_key(RSA_PKCS1_OAEP_PADDING, encrypted, rsa);

    std::cout << "Decrypted message: " << decrypted << std::endl;

    RSA_free(rsa);
    return 0;
}
