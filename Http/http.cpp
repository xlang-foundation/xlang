#include "http.h"
#include "httplib.h"
#include <iterator>
#include <filesystem>
#include <iostream>
#include <string>
#include <map>
#include <tuple>
#include <regex>

namespace fs = std::filesystem;

namespace X
{
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

		auto pre_handler = [this, ProcessRequestUrl](
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
			pSrv->set_pre_routing_handler(pre_handler);
			m_pSrv = (void*)pSrv;
		}
		else
		{
			httplib::Server* pSrv = new httplib::Server();
			if (!pSrv->is_valid())
			{
				printf("server has an error...\n");
			}
			pSrv->set_pre_routing_handler(pre_handler);
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
		std::ifstream file(filePath, std::ios::binary | std::ios::ate);
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
		std::ifstream file(filePath);
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
		else if (path.starts_with("/")) //Stip the leading slash
		{
			path = path.substr(1);
		}
		//Try root first
		for (auto& root : m_staticFileRoots) {
			fs::path fullPath = fs::path(root) / path;
			// Check if this exists
			if (fs::exists(fullPath)) {
				return setResponseContent(fullPath.string());
			}
		}
		//then Moudle Path
		std::string& root = X::Http::I().GetHttpModulePath(); 
		fs::path fullPath = fs::path(root) / path;
		if (fs::exists(fullPath)) {
			return setResponseContent(fullPath.string());
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
	X::Value  HttpRequest::GetBody()
	{
		auto* pReq = (httplib::Request*)m_pRequest;
		std::string strVal = pReq->body;
		return X::Value((char*)strVal.c_str(), (int)strVal.size());
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
		auto* client = static_cast<httplib::SSLClient*>(m_pClient);
		//auto* ctx = client->ssl_context();
		//SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION);
		client->enable_server_certificate_verification(b);
	}
	HttpClient::HttpClient(std::string url)
	{
		// Regular expression to parse the URL
		std::regex url_regex(R"(^(http|https)://([^/:]+)(:\d+)?(/.*)?$)");
		std::smatch url_match_result;

		if (std::regex_match(url, url_match_result, url_regex)) 
		{
			// Extract protocol, host, port, and path
			std::string protocol = url_match_result[1];
			std::string host = url_match_result[2];
			std::string port_str = url_match_result[3];
			std::string path = url_match_result[4];

			if (path.empty()) {
				path = "/";
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
			else if (protocol == "https") {
				m_isHttps = true;
				m_pClient = new httplib::SSLClient(host.c_str(), port);
			}
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
		delete (httplib::Client*)m_pClient;
	}

	bool HttpClient::Get(std::string path)
	{
		char* pDataHead = nullptr;
		char* pBuf = nullptr;
		int buf_size = 0;
		int data_cur_size = 0;
		bool isText = false;
		//if has content length,allocate one time
		//if not, allocated  when data receiving
		httplib::Headers headers;
		//use X::Dict m_headers to fill in headers
		m_headers->Enum([&](X::Value& key, X::Value& value) {
			headers.emplace(key.ToString(), value.ToString());
			});
		auto res = ((httplib::Client*)m_pClient)->Get(
			path, headers,
			[&](const httplib::Response& response) {
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
				int len = 0;
				auto it = response.headers.find("Content-Length");
				if (it != response.headers.end())
				{
					len = std::stoi(it->second);
				}
				if (len > 0)
				{
					pBuf = new char[len];
					pDataHead = pBuf;
					buf_size = len;
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
				// return 'false' if you want to cancel the request.
			},
			[&](const char* data, size_t data_length) {
				if (data_length)
				{
					if (pBuf == nullptr)
					{
						pBuf = new char[data_length];
						pDataHead = pBuf;
						buf_size = data_length;
					}
					else if((data_cur_size + data_length)> buf_size)
					{
						pBuf = new char[data_cur_size + data_length];
						buf_size = data_cur_size + data_length;
						memcpy(pBuf, pDataHead, data_cur_size);
						delete pDataHead;
						pDataHead = pBuf;
						data_cur_size += data_length;
						pBuf += data_length;
					}
					memcpy(pBuf, data, data_length);
					pBuf += data_length;
					data_cur_size += data_length;
				}
				return true; 
				// return 'false' if you want to cancel the request.
			});

		if (!res)
		{
			return false;
		}
		m_status = res->status;

		if (data_cur_size >0)
		{
			if (isText)
			{
				auto* pStr = X::g_pXHost->CreateStr(pDataHead, data_cur_size);
				delete pDataHead;
				m_body = X::Value(pStr, false);
			}
			else
			{
				auto* pBinBuf = X::g_pXHost->CreateBin(pDataHead, data_cur_size, true);
				m_body = X::Value(pBinBuf, false);
			}
		}
		return true;
	}
	bool HttpClient::Post(std::string path, std::string content_type, std::string body)
	{
		if (m_pClient) 
		{
			httplib::Headers headers;
			//use X::Dict m_headers to fill in headers
			m_headers->Enum([&](X::Value& key, X::Value& value) {
				headers.emplace(key.ToString(), value.ToString());
				});
			auto callPost = [&]() {
				if (m_isHttps)
				{
					return ((httplib::SSLClient*)m_pClient)->Post(path, headers, body, content_type);
				}
				else
				{
					return ((httplib::Client*)m_pClient)->Post(path, headers, body, content_type);
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

