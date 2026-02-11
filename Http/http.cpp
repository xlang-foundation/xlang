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
	bool HttpServer::SetAuthenticationCallback(X::Value callback, X::Value parameters)
	{
		m_auth_callback = callback;
		m_auth_parameters = parameters;
		return true;
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
						X::ARGS params(matches.size() - 1 + pat.params.size());
						for (size_t i = 1; i < matches.size(); ++i)
						{
							//std::cout << i << ": '" << matches[i].str() << "'\n";
							std::string strParam(matches[i].str());
							params.push_back(strParam);
						}
						for (X::Value& param : pat.params)
						{
							params.push_back(param);
						}
						X::KWARGS kwargs(pat.kwParams.size() + 2);
						kwargs.Add("req", valReq);
						kwargs.Add("res", valResp);
						for (auto& it : pat.kwParams)
						{
							kwargs.Add(it);
						}
						if (m_auth_callback.IsValid())
						{
							X::ARGS params_cb(3);
							X::List listParams;
							for (auto& arg : params)
							{
								listParams += arg;
							}
							params_cb.push_back(listParams);
							params_cb.push_back(m_auth_parameters);
							params_cb.push_back(pat.strRule);
							X::Value canAccess = m_auth_callback.ObjCall(params_cb, kwargs);
							if (canAccess.IsDict())
							{
								X::Dict dictAccess(canAccess);
								if (!dictAccess->Get("success").ToBool())
								{
									res.status = dictAccess->Get("code").ToInt();
									res.set_content(dictAccess->Get("error").ToString(), "text/plain");
									return true; // request handled
								}
							}
						}
						X::Value retValue;
						bool bCallOK = pat.handler.GetObj()->Call(nullptr,
							pCurPack, params, kwargs, retValue);
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
									pHttpResp->SetContent(v0, pList->Get(1).ToString());
								}
							}
							else
							{
								pHttpResp->SetContent(retValue, "text/html");
							}
						}
						break;
					}
				}
				if (!bHandled)
				{
					if (m_auth_callback.IsValid())
					{
						X::ARGS params_cb(2);
						X::List listParams;
						listParams += url;
						params_cb.push_back(listParams);
						params_cb.push_back(m_auth_parameters);
						X::KWARGS kwargs(2);
						kwargs.Add("req", valReq);
						kwargs.Add("res", valResp);
						X::Value canAccess = m_auth_callback.ObjCall(params_cb, kwargs);
						if (!canAccess.IsTrue())
						{
							res.status = 403;
							res.set_content("Forbidden", "text/plain");
							return true; // request handled
						}
					}
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
				bool bHandled = ProcessRequestUrl(req.path, req, res);
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
				m_cert_path == "" ? nullptr : m_cert_path.c_str(),
				m_private_key_path == "" ? nullptr : m_private_key_path.c_str(),
				m_client_ca_cert_file_path == "" ? nullptr : m_client_ca_cert_file_path.c_str(),
				m_client_ca_cert_dir_path == "" ? nullptr : m_client_ca_cert_dir_path.c_str(),
				m_private_key_password == "" ? nullptr : m_private_key_password.c_str());
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
	bool HttpServer::Listen(std::string srvName, int port, int backlog, int thread_pool_count)
	{
		httplib::Server* pSrv = (httplib::Server*)m_pSrv;
		pSrv->set_listen_backlog(backlog);
		if (thread_pool_count > 0)
		{
			pSrv->set_thread_pool_count(thread_pool_count);
		}
		bool bOK = pSrv->listen(srvName.c_str(), port);
		return bOK;
	}
	bool HttpServer::Stop()
	{
		((httplib::Server*)m_pSrv)->stop();
		return true;
	}
	bool HttpServer::Get(std::string pattern, X::Value& valHandler)
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
						pHandler->Call(nullptr, pCurPack, params0, kwParams0, retValue0);
					}
					catch (int e)
					{
						std::cout << "An exception occurred. Exception Nr. " << e << '\n';
					}
					catch (...)
					{
						std::cout << "An exception occurred." << '\n';
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
		// Add anchors: ^ at start, and (?:\?.*)?$ at end to handle optional query strings
		return "^" + result.str() + "(?:\\?.*)?$";
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
		static std::map<std::string, std::tuple<std::string, bool>> mimeTypeMap =
		{
			// Text files
			{"txt",     {"text/plain", false}},
			{"html",    {"text/html", false}},
			{"htm",     {"text/html", false}},
			{"css",     {"text/css", false}},
			{"csv",     {"text/csv", false}},
			{"xml",     {"application/xml", false}},
			{"xhtml",   {"application/xhtml+xml", false}},
			{"md",      {"text/markdown", false}},
			{"markdown",{"text/markdown", false}},

			// JavaScript
			{"js",      {"application/javascript", false}},
			{"mjs",     {"application/javascript", false}},
			{"json",    {"application/json", false}},
			{"ts",      {"text/typescript", false}},
			{"tsx",     {"text/typescript", false}},

			// Programming Languages - C/C++
			{"c",       {"text/x-c", false}},
			{"h",       {"text/x-c", false}},
			{"cpp",     {"text/x-c++", false}},
			{"cc",      {"text/x-c++", false}},
			{"cxx",     {"text/x-c++", false}},
			{"c++",     {"text/x-c++", false}},
			{"hpp",     {"text/x-c++", false}},
			{"hh",      {"text/x-c++", false}},
			{"hxx",     {"text/x-c++", false}},
			{"h++",     {"text/x-c++", false}},

			// Programming Languages - Other
			{"py",      {"text/x-python", false}},
			{"java",    {"text/x-java", false}},
			{"cs",      {"text/x-csharp", false}},
			{"go",      {"text/x-go", false}},
			{"rs",      {"text/x-rust", false}},
			{"rb",      {"text/x-ruby", false}},
			{"swift",   {"text/x-swift", false}},
			{"kt",      {"text/x-kotlin", false}},
			{"scala",   {"text/x-scala", false}},
			{"r",       {"text/x-r", false}},
			{"m",       {"text/x-objectivec", false}},
			{"mm",      {"text/x-objectivec", false}},

			// Web Assembly & Low Level
			{"wasm",    {"application/wasm", true}},
			{"asm",     {"text/x-asm", false}},
			{"s",       {"text/x-asm", false}},

			// Shell & Scripts
			{"csh",     {"application/x-csh", false}},
			{"sh",      {"application/x-sh", false}},
			{"bash",    {"application/x-sh", false}},
			{"php",     {"application/x-httpd-php", false}},
			{"pl",      {"text/x-perl", false}},
			{"lua",     {"text/x-lua", false}},

			// Config & Data Files
			{"yml",     {"text/yaml", false}},
			{"yaml",    {"text/yaml", false}},
			{"toml",    {"text/toml", false}},
			{"ini",     {"text/plain", false}},
			{"conf",    {"text/plain", false}},
			{"cfg",     {"text/plain", false}},

			// Build & Project Files
			{"cmake",   {"text/x-cmake", false}},
			{"make",    {"text/x-makefile", false}},
			{"gradle",  {"text/x-gradle", false}},

			// Images
			{"jpg",     {"image/jpeg", true}},
			{"jpeg",    {"image/jpeg", true}},
			{"png",     {"image/png", true}},
			{"gif",     {"image/gif", true}},
			{"svg",     {"image/svg+xml", true}},
			{"webp",    {"image/webp", true}},
			{"ico",     {"image/x-icon", true}},
			{"bmp",     {"image/bmp", true}},
			{"tiff",    {"image/tiff", true}},
			{"tif",     {"image/tiff", true}},

			// Audio
			{"mp3",     {"audio/mpeg", true}},
			{"wav",     {"audio/wav", true}},
			{"ogg",     {"audio/ogg", true}},
			{"m4a",     {"audio/mp4", true}},
			{"aac",     {"audio/aac", true}},
			{"weba",    {"audio/webm", true}},

			// Video
			{"mp4",     {"video/mp4", true}},
			{"mpeg",    {"video/mpeg", true}},
			{"webm",    {"video/webm", true}},
			{"avi",     {"video/x-msvideo", true}},
			{"mov",     {"video/quicktime", true}},
			{"wmv",     {"video/x-ms-wmv", true}},

			// Documents
			{"pdf",     {"application/pdf", true}},
			{"doc",     {"application/msword", true}},
			{"docx",    {"application/vnd.openxmlformats-officedocument.wordprocessingml.document", true}},
			{"xls",     {"application/vnd.ms-excel", true}},
			{"xlsx",    {"application/vnd.openxmlformats-officedocument.spreadsheetml.sheet", true}},
			{"ppt",     {"application/vnd.ms-powerpoint", true}},
			{"pptx",    {"application/vnd.openxmlformats-officedocument.presentationml.presentation", true}},
			{"odt",     {"application/vnd.oasis.opendocument.text", true}},
			{"ods",     {"application/vnd.oasis.opendocument.spreadsheet", true}},
			{"odp",     {"application/vnd.oasis.opendocument.presentation", true}},

			// Archives
			{"zip",     {"application/zip", true}},
			{"tar",     {"application/x-tar", true}},
			{"gz",      {"application/gzip", true}},
			{"7z",      {"application/x-7z-compressed", true}},
			{"rar",     {"application/vnd.rar", true}},

			// Fonts
			{"ttf",     {"font/ttf", true}},
			{"otf",     {"font/otf", true}},
			{"woff",    {"font/woff", true}},
			{"woff2",   {"font/woff2", true}},

			// Other common types
			{"bin",     {"application/octet-stream", true}},
			{"exe",     {"application/octet-stream", true}},
			{"rtf",     {"application/rtf", true}},
			{"swf",     {"application/x-shockwave-flash", true}},
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
		namespace fs = std::filesystem;

		fs::path rootPath = GetModulePath(); // assume this is a directory path
		fs::path input(strPath);
		fs::path combined = input.is_absolute() ? input : (rootPath / input);

		std::error_code ec;

		// absolute() does not check existence, and doesn't throw with error_code overload
		fs::path abs = fs::absolute(combined, ec);
		if (ec) {
			// fallback: return best-effort string
			return combined.lexically_normal().string();
		}

		// weakly_canonical won't fail just because the last component doesn't exist
		fs::path normalized = fs::weakly_canonical(abs, ec);
		if (ec) {
			// fallback if something still goes wrong (bad root, permissions, etc.)
			normalized = abs.lexically_normal();
		}

		return normalized.string();
	}
	bool HttpServer::AddRoute(std::string urlPattern, X::Value& func)
	{
		X::ARGS extraParams;
		X::KWARGS extraKw;

		auto url_reg = TranslateUrlToReqex(urlPattern);
		m_patters.push_back(UrlPattern{ urlPattern, std::regex(url_reg), extraParams, extraKw, func });
		return true;
	}
	bool HttpServer::Route(X::XRuntime* rt, X::XObj* pThis, X::XObj* pContext,
		X::ARGS& params, X::KWARGS& kwParams,
		X::Value& trailer, X::Value& retValue)
	{
		X::Value handler = trailer;
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
			int p_size = (int)params.size();
			X::ARGS params1(p_size - 1);
			bool bGotPyHandler = false;
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
					else
					{
						X::XExpr* pExpr = dynamic_cast<X::XExpr*>(pi.GetObj());
						realVal = pExpr->ToKV();
					}
				}
				else if (!bGotPyHandler && pi.IsObject())
				{
					if (pi.GetObj()->GetType() == X::ObjType::PyProxyObject)
					{
						//this case for python decor
						//it is the handler itself
						handler = pi;
						bGotPyHandler = true;
						realVal = pi;
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
			if (bGotPyHandler)
			{
				X::ARGS dummyArgs;
				X::KWARGS dummyKwArgs;
				m_patters.push_back(UrlPattern{ url,std::regex(url_reg),dummyArgs,dummyKwArgs,handler });
			}
			else
			{
				m_patters.push_back(UrlPattern{ url,std::regex(url_reg),params1,kwParams,handler });
			}

		}
		return true;
	}
	X::Value HttpServer::GetRoutes()
	{
		X::List pats;
		for (auto& pat : m_patters)
		{
			X::Dict dict;
			dict->Set("url", pat.strRule);
			X::List params;
			for (auto& arg : pat.params)
			{
				params += arg;
			}
			dict->Set("params", params);
			for (auto& kwarg : pat.kwParams)
			{
				dict->Set(kwarg.key, kwarg.val);
			}
			pats->append(dict);
		}
		return pats;
	}

	bool HttpResponse::AddHeader(std::string headName, X::Value& headValue)
	{
		auto* pResp = (httplib::Response*)m_pResponse;
		pResp->headers.emplace(std::make_pair(headName, headValue.ToString()));
		return true;
	}
	bool HttpResponse::SetContent(X::Value& valContent, std::string contentType)
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
						sink.write(data + offset, length);
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

#define MIN(a, b) ((a) < (b) ? (a) : (b))

	bool HttpResponse::StreamFile(std::string filePath,
		long long start, long long end, std::string contentType)
	{
		auto* pResp = (httplib::Response*)m_pResponse;

		std::cout << "Streaming file: " << filePath << " from " << start << " to " << end << std::endl;
		FILE* file = fopen(filePath.c_str(), "rb");
		if (!file) return false;

		fseek(file, 0, SEEK_END);
		long long file_size = ftell(file);
		if (end < 0 || end >= file_size) end = file_size - 1;

		long long content_length = end - start + 1;
		const size_t CHUNK_SIZE = 256 * 1024;

		pResp->set_content_provider(
			content_length,
			contentType.c_str(),

			// This callback is called ONCE but YOU loop inside to send chunks
			[file, start, end, CHUNK_SIZE](size_t offset, size_t length,
				httplib::DataSink& sink) {
					// Seek to start position
					long long file_position = start + offset;
					fseek(file, file_position, SEEK_SET);

					size_t remaining = length;

					std::cout << "Starting to stream " << length << " bytes from position " << file_position << std::endl;
					// Loop through and send in chunks
					while (remaining > 0) {
						size_t chunk_to_read = MIN(remaining, CHUNK_SIZE);

						std::vector<char> buffer(chunk_to_read);
						size_t bytes_read = fread(buffer.data(), 1, chunk_to_read, file);

						if (bytes_read == 0) {
							break;  // EOF or error
						}

						// Write chunk - if client disconnects, this returns false
						if (!sink.write(buffer.data(), bytes_read)) {
							return false;  // Client disconnected, stop!
						}

						remaining -= bytes_read;

						// Check if client still connected
						if (!sink.is_writable()) {
							return false;  // Stop if client disconnected
						}
					}
					std::cout << "Finished streaming requested data." << std::endl;
					return remaining == 0;  // Success if sent all requested data
			},

			[file](bool success) {
				fclose(file);
			}
		);

		return true;
	}

	// ============================================================================
	// ALTERNATIVE: Stream with Progress Callback (Optional)
	// ============================================================================

	// If you want to track progress or handle errors better:

	struct FileStreamContext {
		FILE* file;
		long long start;
		long long total_sent;
		long long content_length;
		std::string file_path;
	};

	bool HttpResponse::StreamFileWithCallback(std::string filePath,
		long long start, long long end,
		std::string contentType, X::Value progressCallback)
	{
		auto* pResp = (httplib::Response*)m_pResponse;

		// Validate and setup (same as above)
		FILE* temp_file = fopen(filePath.c_str(), "rb");
		if (!temp_file) return false;

		fseek(temp_file, 0, SEEK_END);
		long long file_size = ftell(temp_file);
		fclose(temp_file);

		if (end < 0 || end >= file_size) {
			end = file_size - 1;
		}

		if (start < 0 || start > end) {
			return false;
		}

		long long content_length = end - start + 1;

		FILE* file = fopen(filePath.c_str(), "rb");
		if (!file) return false;

		// Create context
		auto* ctx = new FileStreamContext{
			file,
			start,
			0,
			content_length,
			filePath
		};

		pResp->set_content_provider(
			content_length,
			contentType.c_str(),

			[ctx, progressCallback](size_t offset, size_t length, httplib::DataSink& sink) -> bool {
				long long file_position = ctx->start + offset;

				if (fseek(ctx->file, file_position, SEEK_SET) != 0) {
					return false;
				}

				std::vector<char> buffer(length);
				size_t bytes_read = fread(buffer.data(), 1, length, ctx->file);

				if (bytes_read > 0) {
					sink.write(buffer.data(), bytes_read);
					ctx->total_sent += bytes_read;

					// Optional: Call progress callback
					if (progressCallback.IsObject()) {
						X::ARGS args(2);
						args.push_back((long long)ctx->total_sent);
						args.push_back((long long)ctx->content_length);
						X::KWARGS kwargs;
						X::Value ret;
						progressCallback.GetObj()->Call(nullptr, nullptr, args, kwargs, ret);
					}
				}

				return bytes_read > 0;
			},

			[ctx](bool success) {
				fclose(ctx->file);
				delete ctx;
			}
		);

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

	bool HttpClient::PostWithCallback(std::string path, std::string content_type, std::string body, X::Value callback)
	{
		httplib::Client* client = (httplib::Client*)m_pClient;
		if (!client) {
			return false;
		}

		httplib::Request req;
		req.method = "POST";
		req.path = path;
		
		m_headers->Enum([&](X::Value& key, X::Value& value){
			req.headers.emplace(key.ToString(), value.ToString());
		});

		req.body = body;
		if (!content_type.empty()) {
			req.set_header("Content-Type", content_type);
		}

		req.content_receiver = [callback](const char* data, size_t data_length, uint64_t /*offset*/, uint64_t /*total_length*/) mutable {
			if (callback.IsObject()) {
				std::string chunk(data, data_length);
				X::Value valChunk(chunk);
				X::ARGS args;
				args.push_back(valChunk);
				callback.ObjCall(args);
				return true; 
			}
			return true;
		};

		auto res = client->send(req);
		if (res) {
			m_status = res->status;
			m_body = res->body;

			X::Dict dict;
			for (auto& kv : res->headers)
			{
				X::Str key(kv.first.c_str(), (int)kv.first.size());
				X::Str val(kv.second.c_str(), (int)kv.second.size());
				dict->Set(key, val);
			}
			m_response_headers = dict;
			return true;
		}
		
		return false;
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
	bool HttpClient::MakeHeadersFromString(X::Value& headers)
	{
		std::string strHeaders = headers.ToString();
		X::Dict dict;

		// Split the headers string by semicolon (;)
		std::istringstream headerStream(strHeaders);
		std::string headerPair;

		while (std::getline(headerStream, headerPair, ';'))
		{
			// Trim whitespace from the header pair
			size_t start = headerPair.find_first_not_of(" \t\r\n");
			size_t end = headerPair.find_last_not_of(" \t\r\n");

			if (start == std::string::npos || end == std::string::npos)
				continue; // Skip empty or whitespace-only segments

			headerPair = headerPair.substr(start, end - start + 1);

			// Find the colon separator
			size_t colonPos = headerPair.find(':');
			if (colonPos != std::string::npos && colonPos > 0 && colonPos < headerPair.length() - 1)
			{
				// Extract key and value
				std::string key = headerPair.substr(0, colonPos);
				std::string value = headerPair.substr(colonPos + 1);

				// Trim whitespace from key and value
				start = key.find_first_not_of(" \t\r\n");
				end = key.find_last_not_of(" \t\r\n");
				if (start != std::string::npos && end != std::string::npos)
				{
					key = key.substr(start, end - start + 1);
				}

				start = value.find_first_not_of(" \t\r\n");
				end = value.find_last_not_of(" \t\r\n");
				if (start != std::string::npos && end != std::string::npos)
				{
					value = value.substr(start, end - start + 1);
				}

				// Add to dictionary if both key and value are not empty
				if (!key.empty() && !value.empty())
				{
					X::Str xKey(key.c_str(), (int)key.size());
					X::Str xValue(value.c_str(), (int)value.size());
					dict->Set(xKey, xValue);
				}
			}
		}

		// Set the parsed headers dictionary to m_headers
		m_headers = dict;
		return true;
	}
}


