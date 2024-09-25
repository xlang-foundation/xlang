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

#include "HttpClient.h"
#include <iostream>
#include <string>
#include <regex>
#include <unordered_map>

#if (WIN32)

#include <windows.h>
#include <winhttp.h>

#pragma comment(lib, "winhttp.lib")


namespace X
{
	namespace WebCore
	{
		class HttpRequestHandler
		{
		private:
			HINTERNET hSession = NULL;
			HINTERNET hConnect = NULL;
			HINTERNET hRequest = NULL;
			DWORD dwDownloaded = 0;

			std::string host;
			std::string target;
			std::string port;
			std::unordered_map<std::string, std::string> headers;

			bool is_https;

			std::wstring stringToWString(const std::string& s)
			{
				int len = (int)MultiByteToWideChar(CP_ACP, 0, s.c_str(), (int)s.length(), NULL, 0);
				std::wstring ws(len, L' ');
				MultiByteToWideChar(CP_ACP, 0, s.c_str(), (int)s.length(), &ws[0], len);
				return ws;
			}

			void CloseRequest()
			{
				if (hRequest)
				{
					WinHttpCloseHandle(hRequest);
					hRequest = NULL;
				}
			}
			void OpenRequest()
			{
				if (hConnect && hRequest == NULL)
				{
					auto wstrTarget = stringToWString(target);
					hRequest = WinHttpOpenRequest(hConnect, L"POST",
						wstrTarget.c_str(),
						NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
					if (hRequest)
					{
						for (auto& it : headers) 
						{
							std::string header = it.first + ": " + it.second;
							auto wstrHeader = stringToWString(header);
							WinHttpAddRequestHeaders(hRequest, wstrHeader.c_str(),
								-1L, WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE);
						}
					}
				}
			}
		public:
			HttpRequestHandler(const std::string& url)
			{
				std::regex url_regex(
					R"((https?)://([^:/]+):?(\d*)(/?.*))",
					std::regex_constants::icase
				);
				std::smatch url_match;
				if (std::regex_match(url, url_match, url_regex))
				{
					is_https = url_match[1].str() == "https";
					host = url_match[2].str();
					port = url_match[3].str().empty() ? (is_https ? "443" : "80") : url_match[3].str();
					target = url_match[4].str();
				}
				else
				{
					throw std::runtime_error("Invalid URL format");
				}
				hSession = WinHttpOpen(L"XLang WebCore Http Client/1.0",
					WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
				// Specify an HTTP server.
				if (hSession)
				{
					auto wstrHost = stringToWString(host);
					hConnect = WinHttpConnect(hSession, wstrHost.c_str(),
						INTERNET_DEFAULT_HTTPS_PORT, 0);
				}
			}

			FORCE_INLINE bool setHeader(const std::string name, const std::string value)
			{
				auto it = headers.find(name);
				if (it != headers.end())
				{
					headers.erase(it);
				}
				headers.insert(std::make_pair(name, value));
				return true;
			}

			FORCE_INLINE bool sendRequest(const std::string body_content)
			{
				OpenRequest();
				BOOL bResults = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
					(LPVOID)body_content.c_str(), (DWORD)body_content.length(),
					(DWORD)body_content.length(), 0);
				if (bResults)
				{
					bResults = WinHttpReceiveResponse(hRequest, NULL);
				}
				else
				{
					DWORD dwError = GetLastError();
					dwError = dwError;
				}
				return bResults;
			}
			FORCE_INLINE X::Value readResponse()
			{
				X::Value valData;
				//todo: need to check output MIME 
				DWORD dwSize = 0;
				if (!WinHttpQueryDataAvailable(hRequest, &dwSize))
				{
					CloseRequest();
					return valData;
				}
				if (dwSize == 0)
				{//no more data, return an invalid value to indicate the end of data
					CloseRequest();
					return valData;
				}
				char* pszOutBuffer = new char[dwSize + 1];
				if (!pszOutBuffer)
				{
					CloseRequest();
					return valData;
				}
				else
				{
					ZeroMemory(pszOutBuffer, dwSize + 1);

					if (WinHttpReadData(hRequest, (LPVOID)pszOutBuffer, dwSize, &dwDownloaded))
					{
						std::string strData(pszOutBuffer, dwSize);
						valData = X::Value(strData);
					}
					delete[] pszOutBuffer;
				}
				return valData;
			}
		};

		HttpRequest::HttpRequest(const std::string url)
		{
			m_pImpl = new HttpRequestHandler(url);
		}

		HttpRequest::~HttpRequest()
		{
			if (m_pImpl)
			{
				delete (HttpRequestHandler*)m_pImpl;
			}
		}

		bool HttpRequest::setHeader(const std::string name, const std::string value)
		{
			return ((HttpRequestHandler*)m_pImpl)->setHeader(name, value);
		}
		bool HttpRequest::sendRequest(const std::string body_content)
		{
			return ((HttpRequestHandler*)m_pImpl)->sendRequest(body_content);
		}
		X::Value HttpRequest::readResponse()
		{
			return ((HttpRequestHandler*)m_pImpl)->readResponse();
		}
	}
}
#endif