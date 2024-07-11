from xlang_http import cypher

cypher.StorePath = "XLangStore"
cypher.rsa_padding_mode = cypher.RSA_PKCS1_PADDING
print("RSA_PKCS1_OAEP_PADDING=",cypher.RSA_PKCS1_OAEP_PADDING)
keyName = "test_key_003"
pub_key  = cypher.generate_key_pair(2048,keyName)
print("Public Key: ",pub_key)

msg ="Hello World"

enc_msg = cypher.encrypt_with_private_key(msg,keyName)

msg2 = cypher.decrypt_with_public_key(enc_msg,pub_key)
print("Decrypted Message: ",msg2)

cypher.rsa_padding_mode = cypher.RSA_PKCS1_OAEP_PADDING
msgOther ="another world"
enc_msgOther = cypher.encrypt_with_public_key(msgOther,pub_key)

dec_msgOther = cypher.decrypt_with_private_key(enc_msgOther,keyName)
print("Decrypted Message2: ",dec_msgOther)

cypher.remove_private_key(keyName)
print("end of test...")
