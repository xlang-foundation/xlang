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

		APISET().AddFunc<2>("generate_key_pair", &Cypher::GenerateKeyPair);
		APISET().AddFunc<1>("remove_private_key", &Cypher::RemovePrivateKey);
		APISET().AddVarFunc("encrypt_with_private_key", &Cypher::EncryptWithPrivateKey);
		APISET().AddVarFunc("decrypt_with_private_key", &Cypher::DecryptWithPrivateKey);
		APISET().AddVarFunc("encrypt_with_public_key", &Cypher::EncryptWithPublicKey);
		APISET().AddVarFunc("decrypt_with_public_key", &Cypher::DecryptWithPublicKey);
		END_PACKAGE

	public:
		Cypher();
		~Cypher();
		//return public key
		std::string GenerateKeyPair(int key_size, std::string keyName);
		bool RemovePrivateKey(std::string keyName);
		bool EncryptWithPrivateKey(X::XRuntime* rt, X::XObj* pContext,
			X::ARGS& params, X::KWARGS& kwParams, X::Value& retValue);
		bool DecryptWithPrivateKey(X::XRuntime* rt, X::XObj* pContext,
			X::ARGS& params, X::KWARGS& kwParams, X::Value& retValue);
		bool EncryptWithPublicKey(X::XRuntime* rt, X::XObj* pContext,
			X::ARGS& params, X::KWARGS& kwParams, X::Value& retValue);
		bool DecryptWithPublicKey(X::XRuntime* rt, X::XObj* pContext,
			X::ARGS& params, X::KWARGS& kwParams, X::Value& retValue);
	};
}