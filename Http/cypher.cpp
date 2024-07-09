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
			folderPath = profile_folderPath+storePath+"\\";
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
		folderPath =  std::string(home_dir) + "/"+ storePath+"/";
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

#if (WIN32_PORT)
// Function to generate a self-signed certificate from an RSA key
static X509* generate_certificate(RSA* rsa, const std::string& cert_name) {
	X509* x509 = X509_new();
	X509_set_version(x509, 2);
	ASN1_INTEGER_set(X509_get_serialNumber(x509), 1);

	X509_NAME* name = X509_get_subject_name(x509);
	X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, (unsigned char*)cert_name.c_str(), -1, -1, 0);
	X509_set_issuer_name(x509, name);

	X509_gmtime_adj(X509_get_notBefore(x509), 0);
	X509_gmtime_adj(X509_get_notAfter(x509), 31536000L);  // Valid for 1 year

	EVP_PKEY* pkey = EVP_PKEY_new();
	EVP_PKEY_assign_RSA(pkey, rsa);
	X509_set_pubkey(x509, pkey);

	X509_sign(x509, pkey, EVP_sha256());

	EVP_PKEY_free(pkey);
	return x509;
}

// Function to store the certificate in the Windows Certificate Store
static void store_certificate(X509* x509) {
	PCCERT_CONTEXT pCertContext = NULL;
	BYTE* derCert = NULL;
	int derCertLen = i2d_X509(x509, &derCert);

	if (derCertLen > 0) {
		pCertContext = CertCreateCertificateContext(X509_ASN_ENCODING, derCert, derCertLen);
		if (pCertContext != NULL) {
			HCERTSTORE hCertStore = CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, NULL, CERT_SYSTEM_STORE_CURRENT_USER, L"MY");
			if (hCertStore != NULL) {
				if (!CertAddCertificateContextToStore(hCertStore, pCertContext, CERT_STORE_ADD_NEW, NULL)) {
					// Handle error
				}
				CertCloseStore(hCertStore, 0);
			}
			CertFreeCertificateContext(pCertContext);
		}
		OPENSSL_free(derCert);
	}
}

void store_private_key(RSA* rsa, const std::string& key_name) {
	// Generate a self-signed certificate from the RSA key
	X509* x509 = generate_certificate(rsa, key_name);

	// Store the certificate in the Windows Certificate Store
	store_certificate(x509);

	// Free the certificate
	X509_free(x509);
}

#else
// Function to store the private key (Linux)
void store_private_key(RSA* rsa, const std::string& key_name,std::string& storePath) {
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
#endif

// Function to retrieve the stored private key
RSA* get_stored_private_key(const std::string& key_name,std::string& storePath) {
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
#if (WIN32_PORT)
// Function to encrypt data using a private key associated with a certificate
std::string encrypt_with_private_key_win(const std::string& cert_name, const std::string& data) {
	// Convert the input string to a vector of bytes
	std::vector<BYTE> byte_data(data.begin(), data.end());

	// Open the "MY" certificate store
	HCERTSTORE hCertStore = CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, NULL, CERT_SYSTEM_STORE_CURRENT_USER, L"MY");
	if (!hCertStore) {
		throw std::runtime_error("Failed to open certificate store.");
	}

	// Find the certificate by its subject name
	PCCERT_CONTEXT pCertContext = CertFindCertificateInStore(hCertStore, X509_ASN_ENCODING, 0, CERT_FIND_SUBJECT_STR, std::wstring(cert_name.begin(), cert_name.end()).c_str(), NULL);
	if (!pCertContext) {
		CertCloseStore(hCertStore, 0);
		throw std::runtime_error("Failed to find certificate.");
	}

	// Acquire a handle to the private key
	HCRYPTPROV_OR_NCRYPT_KEY_HANDLE hCryptProvOrNCryptKey = 0;
	DWORD dwKeySpec = 0;
	BOOL fCallerFreeProvOrNCryptKey = FALSE;
	if (!CryptAcquireCertificatePrivateKey(pCertContext, 0, NULL, &hCryptProvOrNCryptKey, &dwKeySpec, &fCallerFreeProvOrNCryptKey)) {
		CertFreeCertificateContext(pCertContext);
		CertCloseStore(hCertStore, 0);
		throw std::runtime_error("Failed to acquire private key handle.");
	}

	// Encrypt the data using the private key
	DWORD dwEncryptedDataLen = 0;
	if (!CryptEncrypt(hCryptProvOrNCryptKey, 0, TRUE, 0, NULL, &dwEncryptedDataLen, 0)) {
		if (fCallerFreeProvOrNCryptKey) CryptReleaseContext(hCryptProvOrNCryptKey, 0);
		CertFreeCertificateContext(pCertContext);
		CertCloseStore(hCertStore, 0);
		throw std::runtime_error("Failed to get encrypted data length.");
	}

	std::vector<BYTE> encrypted_data(dwEncryptedDataLen);
	std::copy(byte_data.begin(), byte_data.end(), encrypted_data.begin());
	if (!CryptEncrypt(hCryptProvOrNCryptKey, 0, TRUE, 0, encrypted_data.data(), &dwEncryptedDataLen, encrypted_data.size())) {
		if (fCallerFreeProvOrNCryptKey) CryptReleaseContext(hCryptProvOrNCryptKey, 0);
		CertFreeCertificateContext(pCertContext);
		CertCloseStore(hCertStore, 0);
		throw std::runtime_error("Failed to encrypt data.");
	}

	// Clean up
	if (fCallerFreeProvOrNCryptKey) CryptReleaseContext(hCryptProvOrNCryptKey, 0);
	CertFreeCertificateContext(pCertContext);
	CertCloseStore(hCertStore, 0);

	// Convert the encrypted data back to a string
	return std::string(encrypted_data.begin(), encrypted_data.end());
}


// Function to encrypt data using a private key associated with a certificate
std::string encrypt_with_private_key(const std::string& key_name, const std::string& data) {
	// Convert the input string to a vector of bytes
	std::vector<BYTE> byte_data(data.begin(), data.end());

	// Open the "MY" certificate store
	HCERTSTORE hCertStore = CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, NULL, CERT_SYSTEM_STORE_CURRENT_USER, L"MY");
	if (!hCertStore) {
		throw std::runtime_error("Failed to open certificate store.");
	}

	// Find the certificate by its subject name
	PCCERT_CONTEXT pCertContext = CertFindCertificateInStore(hCertStore, X509_ASN_ENCODING, 0, CERT_FIND_SUBJECT_STR, std::wstring(key_name.begin(), key_name.end()).c_str(), NULL);
	if (!pCertContext) {
		CertCloseStore(hCertStore, 0);
		throw std::runtime_error("Failed to find certificate.");
	}

	// Get a handle to the private key
	HCRYPTPROV_OR_NCRYPT_KEY_HANDLE hCryptProvOrNCryptKey = 0;
	DWORD dwKeySpec = 0;
	BOOL fCallerFreeProvOrNCryptKey = FALSE;
	if (!CryptAcquireCertificatePrivateKey(pCertContext, 0, NULL, &hCryptProvOrNCryptKey, &dwKeySpec, &fCallerFreeProvOrNCryptKey)) {
		CertFreeCertificateContext(pCertContext);
		CertCloseStore(hCertStore, 0);
		throw std::runtime_error("Failed to acquire private key handle.");
	}

	// Encrypt the data using the private key
	HCRYPTKEY hPrivateKey = (HCRYPTKEY)hCryptProvOrNCryptKey;
	DWORD dwEncryptedDataLen = 0;
	if (!CryptEncrypt(hPrivateKey, 0, TRUE, 0, NULL, &dwEncryptedDataLen, 0)) {
		if (fCallerFreeProvOrNCryptKey) CryptReleaseContext(hCryptProvOrNCryptKey, 0);
		CertFreeCertificateContext(pCertContext);
		CertCloseStore(hCertStore, 0);
		throw std::runtime_error("Failed to get encrypted data length.");
	}

	std::vector<BYTE> encrypted_data(dwEncryptedDataLen);
	std::copy(byte_data.begin(), byte_data.end(), encrypted_data.begin());
	if (!CryptEncrypt(hPrivateKey, 0, TRUE, 0, encrypted_data.data(), &dwEncryptedDataLen, encrypted_data.size())) {
		if (fCallerFreeProvOrNCryptKey) CryptReleaseContext(hCryptProvOrNCryptKey, 0);
		CertFreeCertificateContext(pCertContext);
		CertCloseStore(hCertStore, 0);
		throw std::runtime_error("Failed to encrypt data.");
	}

	// Clean up
	if (fCallerFreeProvOrNCryptKey) CryptReleaseContext(hCryptProvOrNCryptKey, 0);
	CertFreeCertificateContext(pCertContext);
	CertCloseStore(hCertStore, 0);

	// Convert the encrypted data back to a string
	return std::string(encrypted_data.begin(), encrypted_data.end());
}

#else
std::vector<unsigned char> encrypt_with_private_key(const std::string& message, RSA* rsa) {
    std::vector<unsigned char> encrypted(RSA_size(rsa));
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
#endif
// Function to encrypt a message using a public key
std::vector<unsigned char> encrypt_with_public_key(const std::string& message, RSA* rsa) {
	std::vector<unsigned char> encrypted(RSA_size(rsa));
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
	std::string decrypted(RSA_size(rsa), '\0');
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
	std::string decrypted(RSA_size(rsa), '\0');
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
RSA* create_rsa_from_public_key_pem_old(const std::string& public_key_pem) {
	BIO* bio = BIO_new_mem_buf((void*)public_key_pem.c_str(), -1);
	if (!bio) {
		// Handle error
		return nullptr;
	}

	RSA* rsa = PEM_read_bio_RSA_PUBKEY(bio, NULL, NULL, NULL);
	BIO_free(bio);

	if (!rsa) {
		// Handle error
		return nullptr;
	}

	return rsa;
}

// Function to create an RSA structure from a PEM-formatted public key string
RSA* create_rsa_from_public_key_pem(const std::string& public_key_pem) {
	BIO* bio = BIO_new_mem_buf((void*)public_key_pem.c_str(), -1);
	if (!bio) {
		// Handle error
		return nullptr;
	}

	RSA* rsa = RSA_new();
	rsa = PEM_read_bio_RSAPublicKey(bio, &rsa, NULL, NULL);
	BIO_free(bio);

	if (!rsa) {
		// Handle error
		return nullptr;
	}

	return rsa;
}

// Function to remove the stored private key
bool remove_private_key(const std::string& key_name, std::string& storePath) {
	bool bOK = true;
#if (WIN32_PORT)
	// Open the "MY" certificate store.
	HCERTSTORE hCertStore = CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, NULL, CERT_SYSTEM_STORE_CURRENT_USER, L"MY");
	if (!hCertStore) {
		std::cerr << "Failed to open certificate store." << std::endl;
		return false;
	}

	// Find the certificate by its subject name.
	PCCERT_CONTEXT pCertContext = NULL;
	while ((pCertContext = CertFindCertificateInStore(hCertStore, X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, 0, CERT_FIND_SUBJECT_STR, L"My Certificate", pCertContext)) != NULL) {
		// Delete the certificate.
		if (!CertDeleteCertificateFromStore(pCertContext)) {
			std::cerr << "Failed to delete certificate." << std::endl;
			bOK = false;
		}
	}

	// Close the certificate store.
	CertCloseStore(hCertStore, 0);
#else
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
#endif
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
/*
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
*/
//std::string X::Cypher::GenerateKeyPair(int key_size, std::string keyName, std::string storeFolder)
std::string X::Cypher::GenerateKeyPair(int key_size, std::string keyName)
{
	size_t pri_len;
	size_t pub_len;
	char* pri_key = NULL;
	char* pub_key = NULL;

	// Generate RSA key pair
	RSA* rsa = RSA_generate_key(key_size, RSA_F4, NULL, NULL);

	if (rsa == nullptr)
	{
		std::cerr << "Failed to generate RSA key pair." << std::endl;
		return "";
	}
	BIO* pri = BIO_new(BIO_s_mem());
	BIO* pub = BIO_new(BIO_s_mem());
	PEM_write_bio_RSAPrivateKey(pri, rsa, NULL, NULL, 0, NULL, NULL);
	PEM_write_bio_RSAPublicKey(pub, rsa);
	pri_len = BIO_pending(pri);
	pub_len = BIO_pending(pub);
	pri_key = (char*)malloc(pri_len + 1);
	pub_key = (char*)malloc(pub_len + 1);
	BIO_read(pri, pri_key, pri_len);
	BIO_read(pub, pub_key, pub_len);
	pri_key[pri_len] = '\0';
	pub_key[pub_len] = '\0';

	// Write the private key to a file with restricted permissions
	std::string filename = get_private_key_filename(keyName, mStorePath);
	//std::ofstream file(filename, std::ofstream::out| std::ofstream::trunc);
	std::ofstream file(filename, std::ofstream::out | std::ofstream::trunc);
	file.write(pri_key, pri_len);
	file.close();

	//X::Value public_key_pem(X::g_pXHost->CreateBin(pub_key, pub_len, true), false);
	std::string public_key_pem(pub_key, pub_len);
	// Clean up
	RSA_free(rsa);
	BIO_free_all(pub);
	BIO_free_all(pri);
	free(pri_key);
	free(pub_key);

	return public_key_pem;
}

bool X::Cypher::RemovePrivateKey(std::string keyName)
{
	return remove_private_key(keyName,mStorePath);
}

X::Value X::Cypher::EncryptWithPrivateKey(std::string msg, std::string keyName)
{
	RSA* rsa = get_stored_private_key(keyName,mStorePath);
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
	RSA* rsa = get_stored_private_key(keyName,mStorePath);
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

std::string X::Cypher::DecryptWithPrivateKeyG(X::Value& encrypted, std::string keyName)
{
	std::string retMsg;
	std::string filename = get_private_key_filename(keyName, mStorePath);
	//std::ifstream infile(filename, std::ios_base::binary);
	std::ifstream infile(filename);
	// Get the length of the file
	infile.seekg(0, std::ios::end);
	size_t length = infile.tellg();
	infile.seekg(0, std::ios::beg);
	// Create a buffer to hold the file content
	std::vector<char> buffer(length);
	// Read the entire file into the buffer
	infile.read(buffer.data(), length);
	infile.close();

	RSA* rsa = RSA_new();
	BIO* keybio;
	keybio = BIO_new_mem_buf((unsigned char*)buffer.data(), -1);

	rsa = PEM_read_bio_RSAPrivateKey(keybio, &rsa, NULL, NULL);
	if (rsa == nullptr)
	{
		return retMsg;
	}

	// rsa max length
	int key_len = RSA_size(rsa);
	static char stemp[128] = { 0 };
	char* sub_text = stemp;
	bool bNewFlag = false;
	if (key_len > 128)
	{
		sub_text = new char[key_len];
		bNewFlag = true;
	}

	int ret = 0;
	unsigned char* sub_str;
	int pos = 0;
	int desLen = key_len;
	std::vector<unsigned char> vecTemp;
	std::vector<unsigned char> outDataStr;
	X::Bin binEnc(encrypted);
	auto pData = binEnc->Data();
	unsigned char* ptr = (unsigned char*)pData;
	int msg_size = encrypted.Size();
	while (pos < msg_size) {
		ptr += pos;
		sub_str = (unsigned char* )ptr;
		memset(sub_text, 0, key_len);
		ret = RSA_private_decrypt(desLen, sub_str, (unsigned char*)sub_text, rsa, RSA_PKCS1_PADDING);
		if (ret >= 0) {
			//outDataStr.append(std::string(sub_text, ret));
			vecTemp.resize(ret);
			memcpy(vecTemp.data(), sub_text, ret);
			outDataStr.insert(outDataStr.end(), vecTemp.begin(), vecTemp.end());
			pos += desLen;
			desLen = msg_size - pos;
			if (desLen > key_len)
			{
				desLen = key_len;
			}
		}
		else
		{
			break;
		}
	}

	if (bNewFlag)
	{
		delete[] sub_text;
	}
	retMsg = std::string(outDataStr.begin(), outDataStr.end());
	BIO_free_all(keybio);
	RSA_free(rsa);

	return retMsg;

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

X::Value X::Cypher::EncryptWithPublicKeyG(std::string msg, std::string perm_key)
{
	BIO* keybio = BIO_new_mem_buf((unsigned char*)perm_key.c_str(), -1);
	RSA* rsa = RSA_new();
	if (rsa == NULL)
	{
		return false;
	}
	rsa = PEM_read_bio_RSAPublicKey(keybio, &rsa, NULL, NULL);
	if (rsa == NULL)
	{
		return false;
	}

	int key_len = RSA_size(rsa);
	int block_len = key_len - 11;
	static char stemp[128] = { 0 };
	char* sub_text = stemp;

	bool bNewFlag = false;
	if (key_len > 128)
	{
		sub_text = new char[key_len];
		memset(sub_text, 0, key_len);
		bNewFlag = true;
	}

	int ret = 0;
	int pos = 0;
	std::string sub_str;
	const char* pstr = NULL;
	std::vector<unsigned char> vecTemp;
	std::vector<unsigned char> outDataStr;
	int msg_size = msg.size();
	while (pos < msg_size) {
		pstr = msg.c_str() + pos; //clear_text.substr(pos, block_len);

		memset(sub_text, 0, key_len);
		if ((msg_size - pos) >= block_len)
		{
			ret = RSA_public_encrypt(block_len, (const unsigned char*)pstr, (unsigned char*)sub_text, rsa, RSA_PKCS1_PADDING);
		}
		else
		{
			block_len = (msg_size - pos);
			ret = RSA_public_encrypt(block_len, (const unsigned char*)pstr, (unsigned char*)sub_text, rsa, RSA_PKCS1_PADDING);
		}

		if (ret >= 0) {
			vecTemp.resize(ret);
			memcpy(vecTemp.data(), sub_text, ret);
			outDataStr.insert(outDataStr.end(), vecTemp.begin(), vecTemp.end());
		}
		else
		{
			break;
		}
		pos += block_len;
	}//while

	if (bNewFlag)
	{
		delete[] sub_text;
	}
	BIO_free_all(keybio);
	RSA_free(rsa);

	size_t size = outDataStr.size();
	char* pBuf = new char[size];
	memcpy(pBuf, outDataStr.data(), size);
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
	std::vector<unsigned char> ary_encrypted(pData, pData+binEnc.Size());
	std::string msg = decrypt_with_public_key(ary_encrypted, rsa);
	RSA_free(rsa);
	return msg;
}
