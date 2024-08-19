#pragma once
#include "xpackage.h"
#include "xlang.h"


namespace X
{
	class Cypher
	{
		std::string mStorePath;//full path
		std::string mStoreName;//just the folder's name
		int m_rsa_padding_mode = 4;// RSA_PKCS1_OAEP_PADDING;
		bool CheckAndCreateStoreFolder();
	public:
		BEGIN_PACKAGE(Cypher)
			APISET().AddPropL("StorePath", [](auto* pThis, X::Value v) {
			pThis->mStoreName = v.ToString();
			pThis->CheckAndCreateStoreFolder();
				}, [](auto* pThis) {
					return pThis->mStoreName;
					});
		APISET().AddConst("RSA_PKCS1_PADDING",1);
		APISET().AddConst("RSA_SSLV23_PADDING", 2);
		APISET().AddConst("RSA_NO_PADDING", 3);
		APISET().AddConst("RSA_PKCS1_OAEP_PADDING", 4);
		APISET().AddConst("RSA_X931_PADDING", 5);

		APISET().AddPropWithType<int>("rsa_padding_mode", &Cypher::m_rsa_padding_mode);

		APISET().AddFunc<3>("generate_key_pair", &Cypher::GenerateKeyPair);
		APISET().AddFunc<1>("remove_private_key", &Cypher::RemovePrivateKey);
		APISET().AddFunc<2>("encrypt_with_private_key", &Cypher::EncryptWithPrivateKey);
		APISET().AddFunc<2>("decrypt_with_private_key", &Cypher::DecryptWithPrivateKey);
		APISET().AddFunc<2>("encrypt_with_public_key", &Cypher::EncryptWithPublicKey);
		APISET().AddFunc<2>("decrypt_with_public_key", &Cypher::DecryptWithPublicKey);
		END_PACKAGE

	public:
		Cypher();
		~Cypher();
		//return public key
		std::string GenerateKeyPair(int key_size, std::string keyName, std::string storeFolder);
		bool RemovePrivateKey(std::string keyName);
		X::Value EncryptWithPrivateKey(std::string msg, std::string keyName);
		std::string DecryptWithPrivateKey(X::Value& encrypted, std::string keyName);
		X::Value EncryptWithPublicKey(std::string msg, std::string perm_key);
		std::string DecryptWithPublicKey(X::Value& encrypted, std::string perm_key);
	};
}