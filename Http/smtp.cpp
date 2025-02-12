#include "smtp.h"
#include <iostream>
#include <string>
#include <sstream>
#include <stdexcept>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#include <openssl/bio.h>
#include <curl/curl.h>

// Helper function for base64 encoding
std::string base64_encode(const std::string& input) {
	BIO* bio = BIO_new(BIO_f_base64());
	BIO* bmem = BIO_new(BIO_s_mem());
	bio = BIO_push(bio, bmem);

	BIO_write(bio, input.c_str(), input.length());
	BIO_flush(bio);

	BUF_MEM* bptr;
	BIO_get_mem_ptr(bio, &bptr);

	if (bptr == nullptr || bptr->data == nullptr) {
	    BIO_free_all(bio);
	    throw std::runtime_error("Failed to encode base64.");
	}

	std::string encoded(bptr->data, bptr->length - 1); // Avoid last null byte
	BIO_free_all(bio);
    return encoded;
}

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    std::string* response = (std::string*)userp;
    size_t total_size = size * nmemb;
    response->append((char*)contents, total_size);
    return total_size;
}

// Function to get an access token from Microsoft Azure
std::string X::EmailSender::GetAccessToken()
{
    CURL* curl = curl_easy_init();
    if (!curl) 
		throw "Access token failed to initialize";

    std::string url = "https://login.microsoftonline.com/" + mTenantId + "/oauth2/v2.0/token";
    std::string post_fields = "client_id=" + mClientId + "&client_secret=" + mClientSecret + "&grant_type=client_credentials&scope=" + mSmtpScope;

    std::string response_data;

    // SSL Certificate
    curl_easy_setopt(curl, CURLOPT_CAINFO, mCertPath.c_str());

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_fields.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        curl_easy_cleanup(curl);
        return std::string("Access token request failed: " + std::string(curl_easy_strerror(res)));
    }
    curl_easy_cleanup(curl);

    //std::cout << "Response Data: " << response_data << std::endl;

    if (response_data.empty())
    {
        return "Access token response data is empty.";
    }

    size_t start = response_data.find("\"access_token\":\"") + 16;
    size_t end = response_data.find("\"", start);
    if (start == std::string::npos || end == std::string::npos || end <= start) {
        return "Access token failed to parse access token.";
    }

    return response_data.substr(start, end - start);
}

size_t read_callback(void* ptr, size_t size, size_t nmemb, void* userp) {
    std::string* data = static_cast<std::string*>(userp);
    size_t buffer_size = size * nmemb;

    std::cout << "read_callback invoked with buffer_size: " << buffer_size
        << ", remaining data length: " << data->length() << std::endl;

    if (data->empty()) {
        return 0;
    }

    size_t send_length = (data->length() < buffer_size) ? data->length() : buffer_size;
    memcpy(ptr, data->c_str(), send_length);
    data->erase(0, send_length);

    return send_length;
}

struct EmailData {
	std::string from;
	std::vector<std::string> to;
	std::string subject;
	std::string body;

	EmailData(const std::string& from, const std::vector<std::string>& to, const std::string& subject, const std::string& body)
		: from(from), to(to), subject(subject), body(body) {}

	std::string toString() const {
		std::stringstream email_data;
		email_data << "From: " << from << "\r\n"
			<< "Subject: " << subject << "\r\n"
			<< "\r\n"
			<< body << "\r\n";
		return email_data.str();
	}

	struct curl_slist* getMailRcpt() const {
		struct curl_slist* recipients = nullptr;
		for (const auto& recipient : to) {
			recipients = curl_slist_append(recipients, recipient.c_str());
		}
		return recipients;
	}

	const char* getMailFrom() const {
		return from.c_str();
	}
};

// Function to send email using the access token
std::string X::EmailSender::SendEmail(std::string from, std::string to, std::string subject, std::string content)
{
	std::string access_token = GetAccessToken();
	if (access_token.starts_with("Access token")) // get_access_token failed
		return access_token;
	// Create email content
	EmailData emailData(from, { to }, subject, content);
	std::string email_content = emailData.toString();
	if (email_content.empty()) {
		return "Email content is empty.";
	}

	// Encode the access token for SMTP authentication
	std::ostringstream auth_string;
	auth_string << "user=" << emailData.from << "\x01auth=Bearer " << access_token << "\x01\x01";
	std::string auth_string_encoded;

	// Connect to SMTP server
	CURL* curl = curl_easy_init();
	if (!curl) 
		return "Failed to initialize EmailSender";

	char* base64_auth = curl_easy_escape(curl, auth_string.str().c_str(), 0);
	if (!base64_auth) {
		curl_easy_cleanup(curl);
		return "Failed to base64 encode the auth string.";
	}
	auth_string_encoded = base64_auth;
	curl_free(base64_auth);

	//std::cout << "Auth String Encoded: " << auth_string_encoded << std::endl;

	curl_easy_setopt(curl, CURLOPT_URL, ("smtp://" + mSmtpServer + ":" + std::to_string(mSmtpPort)).c_str());
	curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);
	curl_easy_setopt(curl, CURLOPT_USERNAME, emailData.from.c_str());

	curl_easy_setopt(curl, CURLOPT_XOAUTH2_BEARER, auth_string_encoded.c_str());
	//struct curl_slist* auth_headers = nullptr;
	//auth_headers = curl_slist_append(auth_headers, ("AUTH XOAUTH2 " + auth_string_encoded).c_str());
	//curl_easy_setopt(curl, CURLOPT_HTTPHEADER, auth_headers);

	curl_easy_setopt(curl, CURLOPT_MAIL_FROM, emailData.getMailFrom());
	struct curl_slist* recipients = emailData.getMailRcpt();
	curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

	//for big data()
	//curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
	//curl_easy_setopt(curl, CURLOPT_READDATA, &email_content);
	//for small data
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, email_content.c_str());
	curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

	CURLcode res = curl_easy_perform(curl);

	if (res != CURLE_OK) {
		curl_slist_free_all(recipients);
		curl_easy_cleanup(curl);
		return std::string("SMTP request failed: ") + std::string(curl_easy_strerror(res));
	}

	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

	curl_slist_free_all(recipients);
	curl_easy_cleanup(curl);
	return "Email sent successfully!";
}
