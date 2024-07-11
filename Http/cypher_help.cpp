#include <vector>
#include <string>
#include <stdexcept>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <iostream>

// Function to encrypt a message using a private key
std::vector<unsigned char> long_msg_encrypt_with_private_key(int paddingMode, 
    const std::string& message, RSA* rsa) {
    size_t rsa_size = RSA_size(rsa);
    size_t block_size = rsa_size - 11; // Padding reduces the usable block size
    std::vector<unsigned char> encrypted;
    encrypted.reserve(rsa_size * ((message.size() / block_size) + 1));

    for (size_t i = 0; i < message.size(); i += block_size) {
        std::vector<unsigned char> buffer(rsa_size);
        size_t chunk_size = std::min(block_size, message.size() - i);
        int encrypted_length = RSA_private_encrypt(
            chunk_size,
            reinterpret_cast<const unsigned char*>(message.data() + i),
            buffer.data(),
            rsa,
            RSA_PKCS1_PADDING
        );

        if (encrypted_length == -1) {
            ERR_print_errors_fp(stderr);
            throw std::runtime_error("Encryption failed.");
        }

        encrypted.insert(encrypted.end(), buffer.begin(), buffer.begin() + encrypted_length);
    }

    return encrypted;
}

// Function to decrypt a message using a public key
std::string long_msg_decrypt_with_public_key(int paddingMode, 
    const std::vector<unsigned char>& encrypted, RSA* rsa) {
    size_t rsa_size = RSA_size(rsa);
    std::string decrypted;
    decrypted.reserve(encrypted.size());

    for (size_t i = 0; i < encrypted.size(); i += rsa_size) {
        std::vector<unsigned char> buffer(rsa_size);
        int decrypted_length = RSA_public_decrypt(
            rsa_size,
            encrypted.data() + i,
            buffer.data(),
            rsa,
            RSA_PKCS1_PADDING
        );

        if (decrypted_length == -1) {
            ERR_print_errors_fp(stderr);
            throw std::runtime_error("Decryption failed.");
        }

        decrypted.append(reinterpret_cast<char*>(buffer.data()), decrypted_length);
    }

    return decrypted;
}

// Function to encrypt a message using a public key
std::vector<unsigned char> long_msg_encrypt_with_public_key(int paddingMode, 
    const std::string& message, RSA* rsa) {
    size_t rsa_size = RSA_size(rsa);
    size_t block_size = rsa_size - 42; // OAEP Padding reduces the usable block size more
    std::vector<unsigned char> encrypted;
    encrypted.reserve(rsa_size * ((message.size() / block_size) + 1));

    for (size_t i = 0; i < message.size(); i += block_size) {
        std::vector<unsigned char> buffer(rsa_size);
        size_t chunk_size = std::min(block_size, message.size() - i);
        int encrypted_length = RSA_public_encrypt(
            chunk_size,
            reinterpret_cast<const unsigned char*>(message.data() + i),
            buffer.data(),
            rsa,
            RSA_PKCS1_OAEP_PADDING
        );

        if (encrypted_length == -1) {
            ERR_print_errors_fp(stderr);
            throw std::runtime_error("Encryption failed.");
        }

        encrypted.insert(encrypted.end(), buffer.begin(), buffer.begin() + encrypted_length);
    }

    return encrypted;
}

// Function to decrypt a message using a private key
std::string long_msg_decrypt_with_private_key(int paddingMode, 
    const std::vector<unsigned char>& encrypted, RSA* rsa) {
    size_t rsa_size = RSA_size(rsa);
    std::string decrypted;
    decrypted.reserve(encrypted.size());

    for (size_t i = 0; i < encrypted.size(); i += rsa_size) {
        std::vector<unsigned char> buffer(rsa_size);
        int decrypted_length = RSA_private_decrypt(
            rsa_size,
            encrypted.data() + i,
            buffer.data(),
            rsa,
            RSA_PKCS1_OAEP_PADDING
        );

        if (decrypted_length == -1) {
            ERR_print_errors_fp(stderr);
            throw std::runtime_error("Decryption failed.");
        }

        decrypted.append(reinterpret_cast<char*>(buffer.data()), decrypted_length);
    }

    return decrypted;
}

int long_msg_test() {
    // Example usage
    RSA* rsa = RSA_generate_key(2048, RSA_F4, NULL, NULL);

    std::string message = "Hello, this is a test message!";
    std::vector<unsigned char> encrypted = long_msg_encrypt_with_private_key(RSA_PKCS1_OAEP_PADDING,message, rsa);
    std::string decrypted = long_msg_decrypt_with_public_key(RSA_PKCS1_OAEP_PADDING,encrypted, rsa);

    std::cout << "Decrypted message: " << decrypted << std::endl;

    RSA_free(rsa);
    return 0;
}
