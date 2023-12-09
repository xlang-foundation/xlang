//to avoid winsock head file issue
//put all boost include ahead others

#if !(WIN32)

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include "HttpClient.h"
#include <iostream>
#include <string>
#include <regex>

namespace X
{
    namespace WebCore
    {
        namespace http = boost::beast::http;
        class HttpRequestHandler
        {
        private:
            std::string host;
            std::string port;
            bool is_https;
            boost::asio::io_context ioc;
            boost::asio::ssl::context ssl_ctx{boost::asio::ssl::context::tlsv12_client};
            http::request<http::string_body> req;

        public:
            HttpRequestHandler(const std::string& url) : req(http::verb::post, "/", 11)
            {
                std::regex url_regex(
                    R"((https?)://([^:/]+):?(\d*)(/?.*))",
                    std::regex_constants::icase
                );
                std::smatch url_match;
                if (std::regex_match(url, url_match, url_regex)) {
                    is_https = url_match[1].str() == "https";
                    host = url_match[2].str();
                    port = url_match[3].str().empty() ? (is_https ? "443" : "80") : url_match[3].str();
                    req.target(url_match[4].str());

                    if (is_https) {
                        ssl_ctx.set_default_verify_paths();
                    }
                }
                else {
                    throw std::runtime_error("Invalid URL format");
                }
            }

            FORCE_INLINE bool setHeader(const std::string name, const std::string value)
            {
                req.set(name, value);
                return true;
            }

            FORCE_INLINE std::string sendRequest(const std::string body_content)
            {
                try {
                    boost::asio::ip::tcp::resolver resolver{ioc};
                    auto const results = resolver.resolve(host, port);

                    if (is_https) {
                        boost::asio::ssl::stream<boost::beast::tcp_stream> stream{ioc, ssl_ctx};
                        stream.next_layer().connect(results);
                        stream.handshake(boost::asio::ssl::stream_base::client);
                        req.body() = body_content;
                        req.prepare_payload();
                        http::write(stream, req);

                        boost::beast::flat_buffer buffer;
                        http::response<http::dynamic_body> res;
                        http::read(stream, buffer, res);

                        return boost::beast::buffers_to_string(res.body().data());
                    }
                    else {
                        boost::beast::tcp_stream stream{ioc};
                        stream.connect(results);
                        req.body() = body_content;
                        req.prepare_payload();
                        http::write(stream, req);

                        boost::beast::flat_buffer buffer;
                        http::response<http::dynamic_body> res;
                        http::read(stream, buffer, res);

                        return boost::beast::buffers_to_string(res.body().data());
                    }
                }
                catch (std::exception const& e) {
                    std::cerr << "Error: " << e.what() << std::endl;
                    return "";
                }
            }
        };


        int test()
        {
            HttpRequestHandler httpHandler("https://api.openai.com/v1/engines/davinci/completions");
            httpHandler.setHeader("Content-Type", "application/json");
            // You can add more headers using the setHeader method.
            std::string response = httpHandler.sendRequest("{\"prompt\":\"Your prompt here...\",\"max_tokens\":150}");
            std::cout << "Response: " << response << std::endl;
            return 0;
        }

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
            //todo: work with right lib like libcurl
            auto ret =  ((HttpRequestHandler*)m_pImpl)->sendRequest(body_content);
            return true;
        }
        X::Value HttpRequest::readResponse()
        {
            return X::Value();
        }
    }
}

#endif