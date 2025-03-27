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
#include <iostream>
#include <string>
#include <mutex>
#include <thread>
#include <functional>
#include <array>
#include <boost/asio.hpp>

// Assume that X::Value, X::Bin, BEGIN_PACKAGE, END_PACKAGE, and APISET() are provided by XLang.
// For example, X::Value has methods: Size(), Data(), and is callable; and X::Bin wraps a binary buffer.

using boost::asio::ip::udp;

namespace X
{
	class Udp {
	public:
		BEGIN_PACKAGE(Udp)
			APISET().AddFunc<3>("sendPacket", &Udp::sendPacket);
			APISET().AddFunc<1>("setReceiveCallback", &Udp::setReceiveCallback);
		END_PACKAGE
		// Constructor: binds the UDP socket to the given local address and port.
		Udp(const std::string& localAddress, int localPort)
			: io_context_(),
			work_guard_(boost::asio::make_work_guard(io_context_)),
			socket_(io_context_)
		{
			boost::asio::ip::address addr = boost::asio::ip::address::from_string(localAddress);
			udp::endpoint local_endpoint(addr, static_cast<unsigned short>(localPort));
			boost::system::error_code ec;
			socket_.open(local_endpoint.protocol(), ec);
			if (ec) {
				std::cerr << "Socket open error: " << ec.message() << std::endl;
				return;
			}
			socket_.set_option(boost::asio::socket_base::reuse_address(true), ec);
			if (ec) {
				std::cerr << "Set reuse_address error: " << ec.message() << std::endl;
			}
			socket_.bind(local_endpoint, ec);
			if (ec) {
				std::cerr << "Socket bind error: " << ec.message() << std::endl;
			}
			start_receive();
			io_thread_ = std::thread([this]() { io_context_.run(); });
		}

		~Udp() {
			work_guard_.reset();
			io_context_.stop();
			if (io_thread_.joinable())
				io_thread_.join();
		}

		// Sends a UDP packet to the specified remote address and port.
		// 'data' is an X::Value containing binary data.
		bool sendPacket(const std::string& remoteAddress, int remotePort, X::Value& data) {
			X::Bin bin(data);
			const char* ptr = bin->Data();
			size_t size = data.Size();
			std::string packet(ptr, size);
			try {
				udp::endpoint remote(boost::asio::ip::address::from_string(remoteAddress),
					static_cast<unsigned short>(remotePort));
				socket_.async_send_to(boost::asio::buffer(packet), remote,
					[](const boost::system::error_code& ec, std::size_t /*bytes_sent*/) {
						if (ec)
							std::cerr << "Send error: " << ec.message() << std::endl;
					});
				return true;
			}
			catch (const std::exception& e) {
				std::cerr << "sendPacket exception: " << e.what() << std::endl;
				return false;
			}
		}

		// Sets the receive callback.
		// 'callback' is an X::Value representing a callable function.
		// When a packet is received, the callback is invoked with the packet (wrapped as an X::Value).
		void setReceiveCallback(const X::Value& callback) {
			std::lock_guard<std::mutex> lock(callback_mutex_);
			receive_callback_ = callback;
		}

	private:
		// Starts the asynchronous receive loop.
		void start_receive() {
			socket_.async_receive_from(boost::asio::buffer(recv_buffer_), sender_endpoint_,
				[this](const boost::system::error_code& ec, std::size_t bytes_recvd) {
					if (!ec && bytes_recvd > 0) {
						std::string packet(recv_buffer_.data(), bytes_recvd);
						{

							std::lock_guard<std::mutex> lock(callback_mutex_);
							if (receive_callback_) {
								// Wrap the received packet into an X::Bin/X::Value.
								X::Bin bin((char*)nullptr, packet.size(), true);
								memccpy(bin->Data(), packet.data(), 1, packet.size());
								X::Value value(bin);
								// Invoke the callback. (Assuming X::Value is callable.)
								receive_callback_(value);
							}

						}
					}
					else if (ec) {
						std::cerr << "Receive error: " << ec.message() << std::endl;
					}
					start_receive();
				});
		}

		boost::asio::io_context io_context_;
		boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_guard_;
		udp::socket socket_;
		udp::endpoint sender_endpoint_;
		std::array<char, 2048> recv_buffer_;
		std::thread io_thread_;
		std::mutex callback_mutex_;
		X::Value receive_callback_;  // Assumed to be a callable function.
	};

}