#include "cypher.h"

#include <iostream>
#include <fstream>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>

#if (WIN32)
#include <windows.h>
#include <ncrypt.h>
#include <shlobj.h> 
#include <sys/stat.h>  // For chmod on non-Windows
#undef X509_NAME
#include <openssl/x509.h>

#else
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#endif


#if (WIN32)
#include <windows.h>
#include <aclapi.h>
#include <iostream>
#include <string>

#include "cypher_help.h"

bool ChmodWin(const std::string& filename) {
	// Get the SID of the current user
	HANDLE hToken = NULL;
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
		std::cerr << "OpenProcessToken Error: " << GetLastError() << std::endl;
		return false;
	}

	DWORD dwBufferSize = 0;
	GetTokenInformation(hToken, TokenUser, NULL, 0, &dwBufferSize);
	PTOKEN_USER pTokenUser = (PTOKEN_USER)malloc(dwBufferSize);
	if (!GetTokenInformation(hToken, TokenUser, pTokenUser, dwBufferSize, &dwBufferSize)) {
		std::cerr << "GetTokenInformation Error: " << GetLastError() << std::endl;
		CloseHandle(hToken);
		free(pTokenUser);
		return false;
	}

	// Initialize an EXPLICIT_ACCESS structure for an ACE
	EXPLICIT_ACCESS ea;
	ZeroMemory(&ea, sizeof(EXPLICIT_ACCESS));
	ea.grfAccessPermissions = GENERIC_READ | GENERIC_WRITE;
	ea.grfAccessMode = SET_ACCESS;
	ea.grfInheritance = NO_INHERITANCE;
	ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea.Trustee.TrusteeType = TRUSTEE_IS_USER;
	ea.Trustee.ptstrName = (LPTSTR)pTokenUser->User.Sid;

	// Create a new ACL that contains the new ACE
	PACL pACL = NULL;
	if (SetEntriesInAcl(1, &ea, NULL, &pACL) != ERROR_SUCCESS) {
		std::cerr << "SetEntriesInAcl Error: " << GetLastError() << std::endl;
		CloseHandle(hToken);
		free(pTokenUser);
		return false;
	}

	// Initialize a security descriptor
	PSECURITY_DESCRIPTOR pSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
	if (!InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION)) {
		std::cerr << "InitializeSecurityDescriptor Error: " << GetLastError() << std::endl;
		CloseHandle(hToken);
		free(pTokenUser);
		LocalFree(pACL);
		return false;
	}

	// Add the ACL to the security descriptor
	if (!SetSecurityDescriptorDacl(pSD, TRUE, pACL, FALSE)) {
		std::cerr << "SetSecurityDescriptorDacl Error: " << GetLastError() << std::endl;
		CloseHandle(hToken);
		free(pTokenUser);
		LocalFree(pACL);
		LocalFree(pSD);
		return false;
	}

	// Set the security descriptor for the file
	if (!SetFileSecurity(filename.c_str(), DACL_SECURITY_INFORMATION, pSD)) {
		std::cerr << "SetFileSecurity Error: " << GetLastError() << std::endl;
		CloseHandle(hToken);
		free(pTokenUser);
		LocalFree(pACL);
		LocalFree(pSD);
		return false;
	}

	// Clean up
	CloseHandle(hToken);
	free(pTokenUser);
	LocalFree(pACL);
	LocalFree(pSD);

	return true;
}


#endif
// Function to generate RSA key pair
RSA* generate_key_pair(int keySize = 2048) {
	RSA* rsa = RSA_new();
	BIGNUM* e = BN_new();
	BN_set_word(e, RSA_F4);  // Use 65537 as the public exponent
	RSA_generate_key_ex(rsa, keySize, e, nullptr);
	BN_free(e);
	return rsa;
}

// Function to get the public key in PEM format
std::string get_public_key_pem(RSA* rsa) {
	BIO* bio = BIO_new(BIO_s_mem());
	PEM_write_bio_RSA_PUBKEY(bio, rsa);
	char* pem_data;
	long pem_size = BIO_get_mem_data(bio, &pem_data);
	std::string pem(pem_data, pem_size);
	BIO_free(bio);
	return pem;
}

#if (WIN32)
#include <direct.h>
#define MKDIR(path) _mkdir(path)
#else
#include <unistd.h>
#define MKDIR(path) mkdir(path, 0755)
#endif

bool create_directory_recursive(const std::string& path) {

#if (WIN32)
	HRESULT hr = SHCreateDirectoryEx(NULL, path.c_str(), NULL);
	if (hr == ERROR_SUCCESS || hr == ERROR_ALREADY_EXISTS || hr == ERROR_FILE_EXISTS) {
		return true;
	}
	else {
		return false;
	}
#endif
	size_t pos = 0;
	std::string current_path;
	std::string delimiter = "/";
	while ((pos = path.find(delimiter, pos)) != std::string::npos) {
		current_path = path.substr(0, pos);
		if (!current_path.empty()) {
			struct stat info;
			if (stat(current_path.c_str(), &info) != 0) {
				if (MKDIR(current_path.c_str()) != 0) {
					std::cerr << "Failed to create directory: " << current_path << std::endl;
					return false;
				}
			}
			else if (!(info.st_mode & S_IFDIR)) {
				std::cerr << "Path exists but is not a directory: " << current_path << std::endl;
				return false;
			}
		}
		pos += delimiter.length();
	}

	// Create the final directory
	struct stat info;
	if (stat(path.c_str(), &info) != 0) {
		if (MKDIR(path.c_str()) != 0) {
			std::cerr << "Failed to create directory: " << path << std::endl;
			return false;
		}
	}
	else if (!(info.st_mode & S_IFDIR)) {
		std::cerr << "Path exists but is not a directory: " << path << std::endl;
		return false;
	}

	return true;
}
bool IsAbsPath(std::string& strPath)
{
	bool bIsAbs = false;
#if (WIN32)
	if (strPath.find(":/") != std::string::npos
		|| strPath.find(":\\") != std::string::npos
		|| strPath.find("\\\\") != std::string::npos//network path
		|| strPath.find("//") != std::string::npos//network path
		)
	{
		bIsAbs = true;
	}
#else
	if (strPath.find('/') == 0)
	{
		bIsAbs = true;
	}
#endif
	return bIsAbs;
}


std::string get_and_make_store_path(std::string& storePath) {
	std::string folderPath;
	if (IsAbsPath(storePath))
	{
		folderPath = storePath;
	}
	else
	{
#if (WIN32)
		char userProfilePath[MAX_PATH];
		if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_PROFILE, NULL, 0, userProfilePath))) {
			std::string profile_folderPath = std::string(userProfilePath) + "\\AppData\\Roaming\\";
			folderPath = profile_folderPath + storePath + "\\";
		}
		else {
			return "";
		}
#else
		const char* home_dir = getenv("HOME");
		if (!home_dir) {
			struct passwd* pw = getpwuid(getuid());
			home_dir = pw->pw_dir;
		}
		folderPath = std::string(home_dir) + "/" + storePath + "/";
#endif
	}
	bool bOK = create_directory_recursive(folderPath);
	if (bOK)
	{
		return folderPath;
	}
	else
	{
		return "";
	}
}
inline std::string get_private_key_filename(const std::string& key_name, std::string& storePath) {
	return storePath + key_name + "_private_key.pem";
}

// Function to store the private key (Linux)
void store_private_key(RSA* rsa, const std::string& key_name, std::string& storePath) {
	// Get the private key in PEM format
	BIO* bio = BIO_new(BIO_s_mem());
	PEM_write_bio_RSAPrivateKey(bio, rsa, NULL, NULL, 0, NULL, NULL);
	char* pem_data;
	long pem_size = BIO_get_mem_data(bio, &pem_data);

	// Write the private key to a file with restricted permissions
	std::string filename = get_private_key_filename(key_name, storePath);
	std::ofstream file(filename);
	file.write(pem_data, pem_size);
	file.close();

#if (WIN32)
	//ChmodWin(filename); 
#else
	chmod(filename.c_str(), S_IRUSR | S_IWUSR);
#endif

	// Clean up
	BIO_free(bio);
}

// Function to retrieve the stored private key
RSA* get_stored_private_key(const std::string& key_name, std::string& storePath) {
	std::string filename = get_private_key_filename(key_name, storePath);

	BIO* bio = BIO_new_file(filename.c_str(), "rb");
	if (!bio) {
		std::cerr << "Error opening file: " << filename << std::endl;
		return nullptr;
	}
	RSA* rsa = PEM_read_bio_RSAPrivateKey(bio, NULL, NULL, NULL);
	BIO_free(bio);
	if (!rsa) {
		// Handle error: failed to read private key
		return nullptr;
	}
	return rsa;
}

// Fake function to send data to the server (replace with your actual implementation)
void send_to_server(const std::string& data) {
	std::cout << "Sending to server: " << data << std::endl;
}


// Function to encrypt a message using a private key

std::vector<unsigned char> encrypt_with_private_key(const std::string& message, RSA* rsa) {
	auto rsa_size = RSA_size(rsa);
	if (rsa_size < message.size())
	{
		return long_msg_encrypt_with_private_key(message, rsa);
	}
	std::vector<unsigned char> encrypted(rsa_size);
	int encrypted_length = RSA_private_encrypt(
		message.size(),
		reinterpret_cast<const unsigned char*>(message.data()),
		encrypted.data(),
		rsa,
		RSA_PKCS1_PADDING
	);

	if (encrypted_length == -1) {
		ERR_print_errors_fp(stderr);
		throw std::runtime_error("Encryption failed.");
	}

	encrypted.resize(encrypted_length);
	return encrypted;
}

// Function to encrypt a message using a public key
std::vector<unsigned char> encrypt_with_public_key(const std::string& message, RSA* rsa) {
	auto rsa_size = RSA_size(rsa);
	if (rsa_size < message.size())
	{
		return long_msg_encrypt_with_public_key(message, rsa);
	}
	std::vector<unsigned char> encrypted(rsa_size);
	int encrypted_length = RSA_public_encrypt(
		message.size(),
		reinterpret_cast<const unsigned char*>(message.data()),
		reinterpret_cast<unsigned char*>(encrypted.data()),
		rsa,
		RSA_PKCS1_OAEP_PADDING
	);
	if (encrypted_length == -1) {
		ERR_print_errors_fp(stderr);
		exit(EXIT_FAILURE);
	}
	encrypted.resize(encrypted_length);
	return encrypted;
}

// Function to decrypt a message using a public key
std::string decrypt_with_public_key(std::vector<unsigned char>& encrypted, RSA* rsa) {
	auto rsa_size = RSA_size(rsa);
	if (rsa_size < encrypted.size())
	{
		return long_msg_decrypt_with_public_key(encrypted, rsa);
	}
	std::string decrypted(rsa_size, '\0');
	int decrypted_length = RSA_public_decrypt(
		encrypted.size(),
		reinterpret_cast<const unsigned char*>(encrypted.data()),
		reinterpret_cast<unsigned char*>(decrypted.data()),
		rsa,
		RSA_PKCS1_PADDING
	);
	if (decrypted_length == -1) {
		ERR_print_errors_fp(stderr);
		exit(EXIT_FAILURE);
	}
	decrypted.resize(decrypted_length);
	return decrypted;
}

// Function to decrypt a message using a private key
std::string decrypt_with_private_key(std::vector<unsigned char>& encrypted, RSA* rsa) {
	auto rsa_size = RSA_size(rsa);
	if (rsa_size < encrypted.size())
	{
		return long_msg_decrypt_with_private_key(encrypted, rsa);
	}
	std::string decrypted(rsa_size, '\0');
	int decrypted_length = RSA_private_decrypt(
		encrypted.size(),
		reinterpret_cast<const unsigned char*>(encrypted.data()),
		reinterpret_cast<unsigned char*>(decrypted.data()),
		rsa,
		RSA_PKCS1_OAEP_PADDING
	);
	if (decrypted_length == -1) {
		ERR_print_errors_fp(stderr);
		exit(EXIT_FAILURE);
	}
	decrypted.resize(decrypted_length);
	return decrypted;
}

// Function to create an RSA structure from a PEM-formatted public key string

RSA* create_rsa_from_public_key_pem(const std::string& public_key_pem) {
	BIO* bio = BIO_new_mem_buf((void*)public_key_pem.c_str(), -1);
	if (!bio) {
		// Handle error more explicitly, perhaps with a custom log or exception
		return nullptr;
	}

	RSA* rsa = nullptr;  // Initialize RSA pointer to null
	if (!PEM_read_bio_RSA_PUBKEY(bio, &rsa, NULL, NULL)) {
		// Handle error more explicitly here if necessary
		BIO_free(bio);  // Ensure bio is freed in case of failure
		return nullptr;
	}

	BIO_free(bio);  // Free BIO after use
	return rsa;  // rsa should now be properly initialized
}

// Function to remove the stored private key
bool remove_private_key(const std::string& key_name, std::string& storePath) {
	bool bOK = true;

	// Construct the file path for the private key.
	std::string filename = get_private_key_filename(key_name, storePath);
	// Remove the private key file.
#if (WIN32)
	if (!DeleteFile(filename.c_str()))
#else
	if (unlink(filename.c_str()) != 0)
#endif
	{
		std::cerr << "Failed to remove private key file: " << filename << std::endl;
		return false;
	}
	return bOK;
}


int test() {
	// Initialize OpenSSL
	ERR_load_crypto_strings();
	OpenSSL_add_all_algorithms();

	// Generate RSA key pair
	RSA* rsa = generate_key_pair();

	// Key name (used for storing and retrieving the private key)
	std::string key_name = "my_key";
	std::string storePath = "XLang";
	// Store the private key securely
	store_private_key(rsa, key_name, storePath);

	// Retrieve the stored private key
	RSA* retrieved_rsa = get_stored_private_key(key_name, storePath);
	if (!retrieved_rsa) {
		std::cerr << "Failed to retrieve the stored private key." << std::endl;
		return 1;
	}

	// Get the public key in PEM format
	std::string public_key_pem = get_public_key_pem(retrieved_rsa);

	// Send the public key to the server
	send_to_server(public_key_pem);

	// Clean up
	RSA_free(rsa);
	RSA_free(retrieved_rsa);
	EVP_cleanup();
	ERR_free_strings();

	return 0;
}

bool X::Cypher::CheckAndCreateStoreFolder()
{
	std::string path = get_and_make_store_path(mStoreName);
	if (path.empty())
	{
		return false;
	}
	mStorePath = path;
	return true;
}

X::Cypher::Cypher()
{
	// Initialize OpenSSL
	ERR_load_crypto_strings();
	OpenSSL_add_all_algorithms();
}

X::Cypher::~Cypher()
{
	// Clean up OpenSSL
	EVP_cleanup();
	ERR_free_strings();
}

std::string X::Cypher::GenerateKeyPair(int key_size, std::string keyName, std::string storeFolder)
{
	// Generate RSA key pair
	RSA* rsa = generate_key_pair(key_size);
	if (rsa == nullptr)
	{
		std::cerr << "Failed to generate RSA key pair." << std::endl;
		return "";
	}
	std::string public_key_pem = get_public_key_pem(rsa);
	store_private_key(rsa, keyName, mStorePath);
	// Clean up
	RSA_free(rsa);
	return public_key_pem;
}

bool X::Cypher::RemovePrivateKey(std::string keyName)
{
	return remove_private_key(keyName, mStorePath);
}

X::Value X::Cypher::EncryptWithPrivateKey(std::string msg, std::string keyName)
{
	RSA* rsa = get_stored_private_key(keyName, mStorePath);
	if (!rsa)
	{
		std::cerr << "Failed to retrieve the stored private key." << std::endl;
		return "";
	}
	auto encrypted = encrypt_with_private_key(msg, rsa);
	RSA_free(rsa);
	size_t size = encrypted.size();
	char* pBuf = new char[size];
	memcpy(pBuf, encrypted.data(), encrypted.size());
	X::Value valEncrypted(X::g_pXHost->CreateBin(pBuf, size, true), false);
	return valEncrypted;
}

std::string X::Cypher::DecryptWithPrivateKey(X::Value& encrypted, std::string keyName)
{
	RSA* rsa = get_stored_private_key(keyName, mStorePath);
	if (!rsa) {
		std::cerr << "Failed to retrieve the stored private key." << std::endl;
		return "";
	}
	X::Bin binEnc(encrypted);
	auto pData = binEnc->Data();
	std::vector<unsigned char> ary_encrypted(pData, pData + binEnc.Size());
	std::string msg = decrypt_with_private_key(ary_encrypted, rsa);
	RSA_free(rsa);
	return msg;
}

X::Value X::Cypher::EncryptWithPublicKey(std::string msg, std::string perm_key)
{
	RSA* rsa = create_rsa_from_public_key_pem(perm_key);
	if (!rsa) {
		std::cerr << "Failed to create RSA from public key." << std::endl;
		return "";
	}
	auto encrypted = encrypt_with_public_key(msg, rsa);
	RSA_free(rsa);
	size_t size = encrypted.size();
	char* pBuf = new char[size];
	memcpy(pBuf, encrypted.data(), encrypted.size());
	X::Value valEncrypted(X::g_pXHost->CreateBin(pBuf, size, true), false);
	return valEncrypted;
}

std::string X::Cypher::DecryptWithPublicKey(X::Value& encrypted, std::string perm_key)
{
	RSA* rsa = create_rsa_from_public_key_pem(perm_key);
	if (!rsa) {
		std::cerr << "Failed to create RSA from public key." << std::endl;
		return "";
	}
	X::Bin binEnc(encrypted);
	auto pData = binEnc->Data();
	std::vector<unsigned char> ary_encrypted(pData, pData + binEnc.Size());
	std::string msg = decrypt_with_public_key(ary_encrypted, rsa);
	RSA_free(rsa);
	return msg;
}
