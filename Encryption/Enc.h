#pragma once
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <regex>
#include <openssl/rsa.h>
#include <openssl/pem.h>
//#include "xpackage.h"
//#include "xlang.h"
//#include "value.h"
//#include "xhost.h"
//#include "fs.h""

namespace X
{

	class Key 
	{
	public:
		Key()
		{
			mKey = nullptr;
			mIsPublic = true;
		}
		Key(EVP_PKEY* key, bool isPublic)
		{
			mKey = key;
			mIsPublic = isPublic;
		}
		~Key()
		{
			if (mKey)
			{
				delete mKey;
			}
		}
		EVP_PKEY* LoadKeyFromFile(std::string strFile)
		{
			//std::ifstream keyFile(strFile);
			FILE* keyFile = fopen(strFile.c_str(), "r");
			//if (!keyFile.is_open()) {
			if (!keyFile) {
			std::cerr << "Unable to open file for reading." << std::endl;
				return nullptr;
			}
			if (mIsPublic) {
				mKey = PEM_read_PUBKEY(keyFile, nullptr, nullptr, nullptr);
			}
			else
			{
				mKey = PEM_read_PrivateKey(keyFile, nullptr, nullptr, nullptr);
			}
			if (!mKey)
			{
				std::cerr << "Error writing public key to file." << std::endl;
				return nullptr;
			}
			//keyFile.close();
			fclose(keyFile);
			return mKey;
		}
		bool SaveKeyToFile(EVP_PKEY* pkey, std::string strFile)
		{
			mKey = pkey;
			//std::ofstream keyFile(strFile);
			FILE* keyFile = fopen(strFile.c_str(), "w");
			//if (!keyFile.is_open()) {
			if (!keyFile) {
					std::cerr << "Unable to open file for writing." << std::endl;
				return false;
			}

			bool result;
			if (mIsPublic)
			{
				result = PEM_write_PrivateKey(keyFile, mKey, NULL, NULL, 0, 0, NULL);
			}
			else
			{
				result = PEM_write_PUBKEY(keyFile, mKey);
			}
			if (!result)
			{
				std::cerr << "Error writing private key to disk." << std::endl;
				return false;
			}
			//keyFile.close();
			fclose(keyFile);
			return true;
		}
		bool encryptMsg(const std::string& inMsg, std::string& outMsg, EVP_PKEY* pkey) 
		{
			RSA* rsa = EVP_PKEY_get1_RSA(pkey);
			int encLen = RSA_size(rsa);
			int result = -1;
			if (mIsPublic)
			{
				result = RSA_public_encrypt(inMsg.size(), reinterpret_cast<const unsigned char*>(inMsg.c_str()),
					(unsigned char*)outMsg.c_str(), rsa, RSA_PKCS1_PADDING);
			}
			else
			{
				result = RSA_private_encrypt(inMsg.size(), reinterpret_cast<const unsigned char*>(inMsg.c_str()),
					(unsigned char*)outMsg.c_str(), rsa, RSA_PKCS1_PADDING);
			}
			if (result == -1) {
				std::cerr << "Error encrypting the message." << std::endl;
				return false;
			}
			return true;
		}
		bool decryptMsg(const std::string& inMsg, std::string& outMsg, EVP_PKEY* pkey) 
		{
			RSA* rsa = EVP_PKEY_get1_RSA(pkey);
			int decLen = RSA_size(rsa);
			int result = -1;
			if (mIsPublic)
			{
				result = RSA_private_decrypt(inMsg.size(), reinterpret_cast<const unsigned char*>(inMsg.c_str()),
					(unsigned char*)outMsg.c_str(), rsa, RSA_PKCS1_PADDING);
			}
			else
			{
				result = RSA_public_decrypt(inMsg.size(), reinterpret_cast<const unsigned char*>(inMsg.c_str()),
					(unsigned char*)outMsg.c_str(), rsa, RSA_PKCS1_PADDING);
			}
			if (result == -1) 
			{
				std::cerr << "Error decrypting the ciphertext." << std::endl;
				return false;
			}
			return true;
		}
	private:
		EVP_PKEY* mKey;
		bool mIsPublic;
	};

	class ClientSSL
	{
	public:
		//BEGIN_PACKAGE(ClientSSL)
		//	APISET().AddFunc<1>("setPubKeyFile",	&ClientSSL::SetPubKeyFile);
		//	APISET().AddFunc<1>("setPrivKeyFile",	&ClientSSL::SetPrivKeyFile);
		//END_PACKAGE

		ClientSSL() {};
		ClientSSL(std::string& strPubFile, std::string strPrivFile)
		{
			mPubKeyFile = strPubFile;
			mPrivKeyFile = strPrivFile;
		};
		bool GenerateKeys(int bits = 2048) {
			// Generate a new RSA key pair
			RSA* rsa = RSA_generate_key(bits, RSA_F4, nullptr, nullptr);
			if (!rsa) {
				std::cerr << "Error generating RSA key pair." << std::endl;
				return false;
			}

			// Create an EVP_PKEY structure from the RSA key
			EVP_PKEY* pkey = EVP_PKEY_new();
			if (!EVP_PKEY_assign_RSA(pkey, rsa)) {
				std::cerr << "Error creating EVP_PKEY from RSA key." << std::endl;
				return false;
			}

			mPubKey.SaveKeyToFile(pkey, mPubKeyFile);
			mPrivKay.SaveKeyToFile(pkey, mPubKeyFile);

			delete pkey;
			delete rsa;
		}
		bool SetPubKeyFile(std::string& strPubFile)
		{
			mPubKeyFile = strPubFile;
			return true;
		}
		bool SetPrivKeyFile(std::string& strPrivFile)
		{
			mPubKeyFile = strPrivFile;
			return true;
		}
		EVP_PKEY* GetPubKey()
		{
			EVP_PKEY* pkey = mPubKey.LoadKeyFromFile(mPubKeyFile);
			return pkey;
		}
		EVP_PKEY* GetPrivKey()
		{
			EVP_PKEY* pkey = mPrivKay.LoadKeyFromFile(mPubKeyFile);
			return pkey;
		}
	private:
		Key mPrivKay;
		Key mPubKey;
		std::string mPubKeyFile;
		std::string mPrivKeyFile;
	};

	class ServerSSL
	{
	public:
		ServerSSL() {};
		ServerSSL(std::string& strPubFile)
		{
			mPubKeyFile = strPubFile;
		};
		void SetKeyFile(std::string& strPubFile)
		{
			mPubKeyFile = strPubFile;
		}
		bool GetPubKey()
		{
			EVP_PKEY* pkey = mPubKey.LoadKeyFromFile(mPubKeyFile);
			return pkey;
		}
	private:
		Key mPubKey;
		std::string mPubKeyFile;
	};
}//X