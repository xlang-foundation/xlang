#pragma once
#include "xpackage.h"
#include "xlang.h"


namespace X
{
	class Cypher
	{
		std::string mStorePath;//full path
		std::string mStoreName;//just the folder's name

		bool CheckAndCreateStoreFolder();
	public:
		BEGIN_PACKAGE(Cypher)
			APISET().AddPropL("StorePath", [](auto* pThis, X::Value v) {
				pThis->mStoreName = v.ToString();
				pThis->CheckAndCreateStoreFolder();
				}, [](auto* pThis) {
					return pThis->mStoreName;
				});
			APISET().AddFunc<2>("generate_key_pair", &Cypher::GenerateKeyPair);
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
		//std::string GenerateKeyPair(int key_size, std::string keyName, std::string storeFolder);
		std::string GenerateKeyPair(int key_size,std::string keyName);
		bool RemovePrivateKey(std::string keyName);
		X::Value EncryptWithPrivateKey(std::string msg, std::string keyName);
		std::string DecryptWithPrivateKey(X::Value& encrypted, std::string keyName);
		X::Value EncryptWithPublicKey(std::string msg, std::string perm_key);
		std::string DecryptWithPublicKey(X::Value& encrypted, std::string perm_key);
	};
}