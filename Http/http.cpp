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

#include "http.h"
#include "httplib.h"
#include <iterator>
#include <filesystem>
#include <iostream>
#include <string>
#include <map>
#include <tuple>
#include <regex>
#include <optional>
#include <sstream>
#include <iomanip>

namespace fs = std::filesystem;

namespace X
{
	std::tuple<std::string, bool> getMimeTypeAndBinaryFlag(const std::string& extension);

	X::Value Http::WritePad(X::Value& input)
	{
		std::string strInput = input.ToString();
		std::cout << "http:" << input.ToString() << std::endl;
		return input;
	}
	void HttpServer::Init(bool asHttps)
	{
		//valid only for the first matched
		XPackage* pCurPack = APISET().GetProxy(this);
		auto ProcessRequestUrl = [this, pCurPack](std::string url,
			const httplib::Request& req,
			httplib::Response& res)
		{
			bool bHandled = false;

			HttpRequest* pHttpReq = new HttpRequest((void*)&req);
			X::Value valReq(pHttpReq->APISET().GetProxy(pHttpReq));

			HttpResponse* pHttpResp = new HttpResponse(&res);
			X::Value valResp(pHttpResp->APISET().GetProxy(pHttpResp));

			for (auto& pat : m_patters)
			{
				std::smatch matches;
				if (std::regex_search(url, matches, pat.rule))
				{
					X::ARGS params(matches.size()-1+ pat.params.size());
					for (size_t i = 1; i < matches.size(); ++i)
					{
						std::cout << i << ": '" << matches[i].str() << "'\n";
						std::string strParam(matches[i].str());
						params.push_back(strParam);
					}
					for (X::Value& param : pat.params)
					{
						params.push_back(param);
					}
					X::KWARGS kwargs(pat.kwParams.size()+2);
					kwargs.Add("req", valReq);
					kwargs.Add("res", valResp);
					for (auto& it : pat.kwParams)
					{
						kwargs.Add(it);
					}
					X::Value retValue;
					bool bCallOK = pat.handler.GetObj()->Call(nullptr,
						pCurPack, params,kwargs, retValue);
					if (bCallOK)
					{
						bHandled = bCallOK;
					}
					//if get return value, will set content,
					//but only the last set will be valid for respouse
					if (retValue.IsValid())
					{
						if (retValue.IsList()) //[ content,mime]
						{
							XList* pList = dynamic_cast<XList*>(retValue.GetObj());
							if (pList->Size() >= 2)
							{
								X::Value v0 = pList->Get(0);
								pHttpResp->SetContent(v0,pList->Get(1).ToString());
							}
						}
						else
						{
							pHttpResp->SetContent(retValue,"text/html");
						}
					}
					break;
				}
				}
			if (!bHandled)
			{ 
				//if not handled, check if this server support static files
				bHandled = HandleStaticFile(url, (void*)&req, (void*)&res);
			}
			if (!bHandled)
			{
				res.status = 404;
				res.set_content("Not Found", "text/plain");
				bHandled = true;
			}
			return bHandled;
		};

		auto routing_handler_ = [this, ProcessRequestUrl](
			const httplib::Request& req,
			httplib::Response& res)
		{
			auto retCode = httplib::Server::HandlerResponse::Unhandled;
			bool bHandled = ProcessRequestUrl(req.path, req,res);
			if (bHandled)
			{
				retCode = httplib::Server::HandlerResponse::Handled;
			}
			return retCode;
		};
		if (asHttps)
		{
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
			httplib::SSLServer* pSrv = new httplib::SSLServer(
				m_cert_path =="" ? nullptr:m_cert_path.c_str(),
				m_private_key_path =="" ? nullptr:m_private_key_path.c_str(),
				m_client_ca_cert_file_path =="" ?nullptr:m_client_ca_cert_file_path.c_str(),
				m_client_ca_cert_dir_path == ""?nullptr: m_client_ca_cert_dir_path.c_str(),
				m_private_key_password ==""?nullptr: m_private_key_password.c_str());
			if (!pSrv->is_valid())
			{
				printf("server has an error...\n");
			}
			pSrv->set_routing_handler(routing_handler_);
			m_pSrv = (void*)pSrv;
#endif
		}
		else
		{
			httplib::Server* pSrv = new httplib::Server();
			if (!pSrv->is_valid())
			{
				printf("server has an error...\n");
			}
			pSrv->set_routing_handler(routing_handler_);
			m_pSrv = (void*)pSrv;
		}
	}
	HttpServer::~HttpServer()
	{
		for (auto p : m_handlers)
		{
			XFunc* pFuncObj = (XFunc*)p;
			pFuncObj->DecRef();
		}
		m_handlers.clear();
	}
	X::Value HttpServer::GetMimeType(std::string extName)
	{
		auto [mimeType, isBinary] = getMimeTypeAndBinaryFlag(extName);
		X::List list;
		list += mimeType;
		list += isBinary;
		return list;
	}
	bool HttpServer::Listen(std::string srvName,int port)
	{
		httplib::Server* pSrv = (httplib::Server*)m_pSrv;

		//HandlerWithResponse
		bool bOK = ((httplib::Server*)m_pSrv)->listen(
			srvName.c_str(), port);
		return bOK;
	}
	bool HttpServer::Stop()
	{
		((httplib::Server*)m_pSrv)->stop();
		return true;
	}
	bool HttpServer::Get(std::string pattern,X::Value& valHandler)
	{
		XFunc* pHandler = nullptr;
		if (valHandler.IsObject())
		{
			auto* pFuncObj = valHandler.GetObj();
			if (pFuncObj->GetType() == X::ObjType::Function)
			{
				pHandler = dynamic_cast<XFunc*>(pFuncObj);
				pHandler->IncRef();//keep for lambda
				m_handlers.push_back((void*)pHandler);
			}
		}
		XPackage* pCurPack = APISET().GetProxy(this);
		((httplib::Server*)m_pSrv)->Get(pattern,
			[pHandler, pCurPack](const httplib::Request& req,
				httplib::Response& res)
			{
				if (pHandler)
				{
					ARGS params0(2);
					HttpRequest* pHttpReq = new HttpRequest((void*)&req);
					params0.push_back(X::Value(pHttpReq->APISET().GetProxy(pHttpReq)));

					HttpResponse* pHttpResp = new HttpResponse(&res);
					params0.push_back(X::Value(pHttpResp->APISET().GetProxy(pHttpResp)));

					KWARGS kwParams0;
					X::Value retValue0;
					try 
					{
						pHandler->Call(nullptr,pCurPack,params0, kwParams0,retValue0);
					}
					catch (int e)
					{
						std::cout << "An exception occurred. Exception Nr. " << e << '\n';
					}
					catch (...)
					{
						std::cout << "An exception occurred."<< '\n';
					}
				}
			});
		return true;
	}
	std::string HttpServer::TranslateUrlToReqex(std::string& url)
	{
		if (url == "/")
		{
			return "^/$";
		}
		std::string pattern = "<[^>]*>";
		const std::regex r(pattern);
		std::string target = "([^/&\\?]*)";
		std::stringstream result;
		std::regex_replace(std::ostream_iterator<char>(result),
			url.begin(), url.end(), r, target);
		return result.str();
	}

	// Function to extract the file extension from a file path
	std::string getFileExtension(const std::string& filePath) 
	{
		size_t dotPos = filePath.find_last_of('.');
		if (dotPos == std::string::npos) 
		{
			return ""; // No extension found
		}
		return filePath.substr(dotPos + 1);
	}

	// Function to get the MIME type and binary/text indicator based on the file extension
	std::tuple<std::string, bool> getMimeTypeAndBinaryFlag(const std::string& extension) {
		// Mapping of file extensions to MIME types and binary/text flag
		std::map<std::string, std::tuple<std::string, bool>> mimeTypeMap = 
		{
			{"txt",		{"text/plain", false}},
			{"html",	{"text/html", false}},
			{"css",		{"text/css", false}},
			{"js",		{"application/javascript", false}},
			{"json",	{"application/json", false}},
			{"csh",		{"application/x-csh", false}},
			{"sh",		{"application/x-sh", false}},
			{"php",		{"application/x-httpd-php", false}},
			{"xml",		{"application/xml", false}},
			{"xhtml",	{"application/xhtml+xml", false}},
			{"jpg",		{"image/jpeg", true}},
			{"jpeg",	{"image/jpeg", true}},
			{"png",		{"image/png", true}},
			{"gif",		{"image/gif", true}},
			{"svg",		{"image/svg+xml", true}},
			{"pdf",		{"application/pdf", true}},
			// Add more mappings as needed
		};

		// Find the MIME type and binary/text flag based on the extension
		auto it = mimeTypeMap.find(extension);
		if (it != mimeTypeMap.end()) 
		{
			return it->second;
		}

		// Return a default MIME type and binary flag if the extension is not recognized
		return { "application/octet-stream", true };
	}
	// Function to read the entire contents of a binary file
	std::vector<char> BinReadAll(const std::string& filePath) 
	{
		std::filesystem::path fsPath = std::filesystem::u8path(filePath);
		std::ifstream file(fsPath, std::ios::binary | std::ios::ate);
		if (!file) 
		{
			throw std::runtime_error("Could not open file for reading: " + filePath);
		}
		std::streamsize size = file.tellg();
		file.seekg(0, std::ios::beg);

		std::vector<char> buffer(size);
		if (!file.read(buffer.data(), size)) 
		{
			throw std::runtime_error("Could not read file: " + filePath);
		}
		return buffer;
	}

	// Function to read the entire contents of a text file
	std::string TextReadAll(const std::string& filePath) 
	{
		std::filesystem::path fsPath = std::filesystem::u8path(filePath);
		std::ifstream file(fsPath);
		if (!file) 
		{
			throw std::runtime_error("Could not open file for reading: " + filePath);
		}
		std::string content((std::istreambuf_iterator<char>(file)),
			std::istreambuf_iterator<char>());
		return content;
	}

	bool HttpServer::HandleStaticFile(std::string path, void* pReq, void* pResp) 
	{
		if (!m_SupportStaticFiles)
		{
			return false;
		}

		auto setResponseContent = [&](const std::string& filePath) {
			std::string extension = getFileExtension(filePath);
			auto [mimeType, isBinary] = getMimeTypeAndBinaryFlag(extension);

			try {
				if (isBinary) {
					std::vector<char> data = BinReadAll(filePath);
					((httplib::Response*)pResp)->set_content(std::string(data.begin(), data.end()), mimeType.c_str());
				}
				else {
					std::string data = TextReadAll(filePath);
					((httplib::Response*)pResp)->set_content(data, mimeType.c_str());
				}
				return true;
			}
			catch (const std::exception& e) {
				// Handle file reading errors
				((httplib::Response*)pResp)->status = 500; // Internal Server Error
				((httplib::Response*)pResp)->set_content("Error reading file: " + std::string(e.what()), "text/plain");
				return false;
			}
		};

		if (path == "/")
		{
			path = m_staticIndexFile;
		}
		else if (path.starts_with("/")) //Strip the leading slash
		{
			path = path.substr(1);
		}
		std::filesystem::path fsPath = std::filesystem::u8path(path);
		//Try root first
		for (auto& root : m_staticFileRoots) {
			fs::path fullPath = fs::path(root) / fsPath;
			// Check if this exists
			if (fs::exists(fullPath)) {
				std::u8string u8Str = fullPath.u8string();
				std::string filePath(reinterpret_cast<const char*>(u8Str.data()), u8Str.size());
				return setResponseContent(filePath);
			}
		}
		//then Moudle Path
		std::string& root = X::Http::I().GetHttpModulePath(); 
		fs::path fullPath = fs::path(root) / path;
		if (fs::exists(fullPath)) {
			std::u8string u8Str = fullPath.u8string();
			std::string filePath(reinterpret_cast<const char*>(u8Str.data()), u8Str.size());
			return setResponseContent(filePath);
		}

		return false;
	}
	std::string HttpServer::GetModulePath()
	{
		std::string modulePath;
		X::XRuntime* pRt = X::g_pXHost->GetCurrentRuntime();
		if (pRt)
		{
			X::Value varModulePath = pRt->GetXModuleFileName();
			if (varModulePath.IsValid())
			{
				std::string strModulePath = varModulePath.ToString();
				if (!strModulePath.empty())
				{
					fs::path filePath(strModulePath);
					modulePath = filePath.parent_path().string();
				}
			}
			else
			{
				//todo:
				modulePath = X::Http::I().GetModulePath();
			}
		}
		else
		{
			modulePath = X::Http::I().GetModulePath();
		}
		return modulePath;
	}

	std::string HttpServer::ConvertReletivePathToFullPath(std::string strPath)
	{
		std::string modulePath = GetModulePath();
		std::filesystem::path rootPath = modulePath;
		std::filesystem::path fullPath;
		// Check if the path is already absolute
		if (std::filesystem::path(strPath).is_absolute())
		{
			fullPath = std::filesystem::canonical(strPath);
		}
		else {
			// Combine the paths and normalize
			fullPath = std::filesystem::absolute(rootPath / strPath);
			fullPath = std::filesystem::canonical(fullPath);
		}
		return fullPath.string();
	}

	bool HttpServer::Route(X::XRuntime* rt,X::XObj* pThis,X::XObj* pContext,
		X::ARGS& params, X::KWARGS& kwParams,
		X::Value& trailer, X::Value& retValue)
	{
		//if Route is working in decor mode, trailer will be the orgin function
		//first parameter is the url
		if (params.size() > 0)
		{
			std::string url;
			X::Value& p0 = params[0];
			if (p0.IsObject() && p0.GetObj()->GetType() == ObjType::Expr)
			{
				X::ARGS params0(0);
				X::KWARGS kwParams0;
				X::Value exprValue;
				if (p0.GetObj()->Call(rt, pContext, params0, kwParams0, exprValue))
				{
					url = exprValue.ToString();
				}
			}
			else
			{
				url = p0.ToString();
			}
			int p_size = (int)params.size()-1;
			X::ARGS params1(p_size);
			for (int i = 1; i < p_size; i++)
			{
				X::Value realVal;
				X::Value& pi = params[i];
				if (pi.IsObject() && pi.GetObj()->GetType() == ObjType::Expr)
				{
					X::ARGS params0(0);
					X::KWARGS kwParams0;
					X::Value exprValue;
					if (pi.GetObj()->Call(rt, pContext, params0, kwParams0, exprValue))
					{
						realVal = exprValue;
					}
				}
				else
				{
					realVal = pi;
				}
				params1.push_back(realVal);
			}
			params1.Close();
			auto url_reg = TranslateUrlToReqex(url);
			m_patters.push_back(UrlPattern{ url,std::regex(url_reg),params1,kwParams,trailer });

		}
		return true;
	}
	bool HttpResponse::AddHeader(std::string headName, X::Value& headValue)
	{
		auto* pResp = (httplib::Response*)m_pResponse;
		pResp->headers.emplace(std::make_pair(headName,headValue.ToString()));
		return true;
	}
	bool HttpResponse::SetContent(X::Value& valContent,std::string contentType)
	{
		auto* pResp = (httplib::Response*)m_pResponse;
		if (valContent.IsObject())
		{
			auto* pObjContent = valContent.GetObj();
			if (pObjContent->GetType() == X::ObjType::Binary)
			{
				auto* pBinContent = dynamic_cast<XBin*>(pObjContent);
				pBinContent->IncRef();
				pResp->set_content_provider(
					pBinContent->Size(), // Content length
					contentType.c_str(), // Content type
					[pBinContent](size_t offset, size_t length, 
						httplib::DataSink& sink) 
					{
						char* data = pBinContent->Data();
						sink.write(data+offset,length);
						return true;
					},
					[pBinContent](bool success) 
					{ 
						pBinContent->DecRef();
					}
					);
			}
			else
			{
				pResp->set_content(valContent.ToString().c_str(),
					contentType.c_str());
			}
		}
		else
		{
			pResp->set_content(valContent.ToString().c_str(),
				contentType.c_str());
		}
		return true;
	}
	X::Value HttpRequest::GetMethod()
	{
		auto* pReq = (httplib::Request*)m_pRequest; 
		return X::Value(pReq->method);
	}

	// Function to determine if the MIME type is binary or textual
	inline bool isBinaryContentType(const std::string& content_type) 
	{
		// Set of known textual MIME types (extend as needed)
		static const std::unordered_set<std::string> textMimeTypes = {
			"text/plain", "text/html", "text/css", "text/javascript",
			"application/json", "application/xml", "application/x-www-form-urlencoded"
		};

		// If content type starts with "text/", it's likely textual
		if (content_type.find("text/") == 0) {
			return false; // Not binary, it's text
		}

		// If content type is in the predefined textual set, it's text
		if (textMimeTypes.find(content_type) != textMimeTypes.end()) {
			return false; // Not binary, it's text
		}

		// For other known cases, check if it's binary (can extend the list)
		if (content_type.find("image/") == 0 || // Image files
			content_type.find("audio/") == 0 || // Audio files
			content_type.find("video/") == 0 || // Video files
			content_type == "application/octet-stream") { // Generic binary stream
			return true; // It's binary
		}

		// Default fallback for unknown content types
		return true; // Assume it's binary if unknown
	}
	inline std::optional<std::string> getContentType(auto& headers) 
	{
		auto it = headers.find("Content-Type");
		if (it != headers.end()) 
		{
			return it->second; // Return the content type value
		}
		return std::nullopt; // No content-type found
	}
	X::Value  HttpRequest::GetBody()
	{
		X::Value retVal;
		auto* pReq = (httplib::Request*)m_pRequest;
		if (pReq->body.empty())
		{
			X::List listBody;
			//check files for MultipartFormDataMap files;
			for (const auto& pair : pReq->files)
			{
				X::Dict dataMap;
				const std::string& key = pair.first;
				auto& value = pair.second;
				dataMap->Set("name", value.name);
				bool isBin = isBinaryContentType(value.content_type);
				if (isBin)
				{
					X::Bin binContent((char*)nullptr, 
						(unsigned long long)value.content.size(), 
						static_cast<bool>(true));
					memcpy(binContent->Data(), value.content.data(), value.content.size());
					dataMap->Set("content", binContent);
				}
				else
				{
					dataMap->Set("content", value.content);
				}
				dataMap->Set("filename", value.filename);
				dataMap->Set("content_type", value.content_type);
				listBody += dataMap;
			}
			retVal = listBody;

		}
		else
		{
			bool isBin = true;
			std::string& strVal = pReq->body;
			if (auto content_type = getContentType(pReq->headers))
			{
				isBin = isBinaryContentType(*content_type);
			}
			if (isBin)
			{
				X::Bin binContent((char*)nullptr, 
					(unsigned long long)strVal.size(),
					static_cast<bool>(true));
				memcpy(binContent->Data(), strVal.data(), strVal.size());
				retVal = binContent;
			}
			else
			{
				retVal = strVal;
			}
		}
		return retVal;
	}
	X::Value  HttpRequest::GetPath()
	{
		auto* pReq = (httplib::Request*)m_pRequest;
		return X::Value(pReq->path);
	}
	X::Value  HttpRequest::Get_remote_addr()
	{
		auto* pReq = (httplib::Request*)m_pRequest;
		std::string strVal = pReq->remote_addr;
		return X::Value(pReq->remote_addr);
	}
	X::Value  HttpRequest::GetAllHeaders()
	{
		auto* pReq = (httplib::Request*)m_pRequest;
		auto& headers = pReq->headers;
		X::Dict dict;
		for (auto it = headers.begin(); it != headers.end(); ++it)
		{
			const auto& x = *it;
			X::Str key(x.first.c_str(), (int)x.first.size());
			X::Str val(x.second.c_str(), (int)x.second.size());
			dict->Set(key, val);
		}
		return dict;
	}
	X::Value  HttpRequest::GetParams()
	{
		auto* pReq = (httplib::Request*)m_pRequest;
		auto& req_params = pReq->params;
		X::Dict dict;
		for (auto it = req_params.begin();
			it != req_params.end(); ++it)
		{
			const auto& x = *it;
			X::Str key(x.first.c_str(), (int)x.first.size());
			X::Str val(x.second.c_str(), (int)x.second.size());
			dict->Set(key, val);
		}
		return dict;
	}
	void HttpClient::set_enable_server_certificate_verification(bool b)
	{
		if (!m_isHttps)
		{
			return;
		}
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
		auto* client = static_cast<httplib::SSLClient*>(m_pClient);
		//auto* ctx = client->ssl_context();
		//SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION);
		client->enable_server_certificate_verification(b);
#endif
	}
	HttpClient::HttpClient(std::string url)
	{
		// Regular expression to parse the URL
		std::regex url_regex(R"(^(http|https)://([^/:]+)(?::(\d+))?(/.*)?$)");
		std::smatch url_match_result;

		if (std::regex_match(url, url_match_result, url_regex)) 
		{
			// Extract protocol, host, port, and path
			std::string protocol = url_match_result[1];
			std::string host = url_match_result[2];
			std::string port_str = url_match_result[3];
			std::string path = url_match_result[4];

			if (!path.empty()) {
				m_path = path;
			}

			int port = 0;
			if (!port_str.empty()) {
				port = std::stoi(port_str);
			}
			else {
				port = (protocol == "https") ? 443 : 80;
			}

			if (protocol == "http") {
				m_isHttps = false;
				m_pClient = new httplib::Client(host.c_str(), port);
			}
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
			else if (protocol == "https") {
				m_isHttps = true;
				m_pClient = new httplib::SSLClient(host.c_str(), port);
			}
#endif
			else {
				throw std::runtime_error("Unsupported protocol: " + protocol);
			}
		}
		else {
			throw std::runtime_error("Invalid URL format: " + url);
		}
	}
	HttpClient::~HttpClient()
	{
		if (m_isHttps)
		{
#if CPPHTTPLIB_OPENSSL_SUPPORT
			delete (httplib::SSLClient*)m_pClient;
#endif
		}
		else
		{
			delete (httplib::Client*)m_pClient;
		}
	}

	bool HttpClient::Get(std::string path)
	{
		bool isText = false;
		int expectedContentLength = 0;
		X::XBin* pBin = nullptr;
		char* pCurrentPos = nullptr;
		size_t totalReceivedSize = 0;

		httplib::Headers headers;
		//use X::Dict m_headers to fill in headers
		m_headers->Enum([&](X::Value& key, X::Value& value) {
			headers.emplace(key.ToString(), value.ToString());
			});

		auto response_handler = [&](const httplib::Response& response) {
			auto it0 = response.headers.find("Content-Type");
			if (it0 != response.headers.end())
			{
				if ((it0->second.find("text/") != it0->second.npos) ||
					(it0->second.find("application/x-javascript") != it0->second.npos) ||
					(it0->second.find("application/x-csh") != it0->second.npos) ||
					(it0->second.find("application/x-sh") != it0->second.npos) ||
					(it0->second.find("application/json") != it0->second.npos) ||
					(it0->second.find("application/xml") != it0->second.npos) ||
					(it0->second.find("application/xhtml+xml") != it0->second.npos) ||
					(it0->second.find("application/x-httpd-php") != it0->second.npos))
				{
					isText = true;
				}
			}

			auto it = response.headers.find("Content-Length");
			if (it != response.headers.end())
			{
				expectedContentLength = std::stoi(it->second);
				// If we know the size, pre-allocate the XBin regardless of content type
				if (expectedContentLength > 0) {
					pBin = X::g_pXHost->CreateBin(nullptr, expectedContentLength, true);
					pCurrentPos = pBin->Data();
				}
			}

			X::Dict dict;
			//dump response headers
			for (auto& kv : response.headers)
			{
				X::Str key(kv.first.c_str(), (int)kv.first.size());
				X::Str val(kv.second.c_str(), (int)kv.second.size());
				dict->Set(key, val);
			}
			m_response_headers = dict;
			return true;
			};

		auto content_receiver = [&](const char* data, size_t data_length) {
			if (data_length)
			{
				// For both text and binary data, handle the same way
				if (pBin == nullptr) {
					// First chunk or didn't know content length
					pBin = X::g_pXHost->CreateBin(nullptr, data_length, true);
					pCurrentPos = pBin->Data();
				}
				else if (totalReceivedSize + data_length > pBin->Size()) {
					// Need to resize
					size_t newSize = totalReceivedSize + data_length;
					X::XBin* pNewBin = X::g_pXHost->CreateBin(nullptr, newSize, true);
					char* pNewData = pNewBin->Data();

					// Copy existing data
					memcpy(pNewData, pBin->Data(), totalReceivedSize);
					// Release the old bin
					pBin->DecRef();

					// Update pointers
					pBin = pNewBin;
					pCurrentPos = pNewData + totalReceivedSize;
				}

				// Copy new data directly to the XBin buffer
				memcpy(pCurrentPos, data, data_length);
				pCurrentPos += data_length;
				totalReceivedSize += data_length;
			}
			return true;
			};

		std::string full_path = m_path + path;
		auto call = [&]() {
			if (m_isHttps) {
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
				return ((httplib::SSLClient*)m_pClient)->Get(full_path, headers,
					response_handler, content_receiver);
#endif
			}
			else {
				return ((httplib::Client*)m_pClient)->Get(full_path, headers,
					response_handler, content_receiver);
			}
			};

		auto res = call();
		if (!res)
		{
			// Clean up if request failed
			if (pBin != nullptr) {
				pBin->DecRef();
			}
			return false;
		}

		m_status = res->status;

		// Process the collected data
		if (pBin != nullptr && totalReceivedSize > 0)
		{
			if (isText)
			{
				// For text, create string directly from the XBin data
				auto* pStr = X::g_pXHost->CreateStr(pBin->BorrowDta(), totalReceivedSize);
				m_body = X::Value(pStr, false);
				// Release the XBin as we no longer need it
				pBin->DecRef();
			}
			else
			{
				// For binary, we already have the data in the XBin
				m_body = X::Value(pBin, false);
			}
		}

		return true;
	}

	bool HttpClient::Post(std::string path, std::string content_type, std::string body)
	{
		if (m_pClient) 
		{
			std::string full_path = m_path + path;
			httplib::Headers headers;
			//use X::Dict m_headers to fill in headers
			m_headers->Enum([&](X::Value& key, X::Value& value) {
				headers.emplace(key.ToString(), value.ToString());
				});
			auto callPost = [&]() {
				if (m_isHttps)
				{
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
					return ((httplib::SSLClient*)m_pClient)->Post(full_path, headers, body, content_type);
#endif
				}
				else
				{
					return ((httplib::Client*)m_pClient)->Post(full_path, headers, body, content_type);
				}
			};
			httplib::Result res = callPost();
			if (res) 
			{
				m_status = res->status;
				m_body = X::Value(res->body); 
				X::Dict dict;
				//dump response headers
				for (auto& kv : res->headers)
				{
					X::Str key(kv.first.c_str(), (int)kv.first.size());
					X::Str val(kv.second.c_str(), (int)kv.second.size());
					dict->Set(key, val);
				}
				m_response_headers = dict;
				return true;
			}
		}
		return false;
	}
	X::Value HttpClient::GetStatus()
	{
		return m_status;
	}
	X::Value HttpClient::GetBody()
	{
		return m_body;
	}
}

