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
#include "xpackage.h"
#include "xlang.h"
#include "SerialPort.h"
#include <string>
#include <memory>
#include <vector>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <atomic>
#include <unordered_map>

namespace X
{
	namespace BareLink
	{
		enum CommandType { LoadCode, RunCode, StopCodeRun };

		class Device
		{
		public:
			BEGIN_PACKAGE(Device)
				APISET().AddFunc<0>("Connect", &Device::Connect);
			APISET().AddFunc<0>("Disconnect", &Device::Disconnect);
			APISET().AddVarFunc("RunCommand", &Device::RunCommand);
			END_PACKAGE

				Device(std::string deviceId);
			bool Connect();
			bool Disconnect();
			bool RunCommand(
				X::XRuntime* rt, X::XObj* pContext,
				X::ARGS& params, X::KWARGS& kwParams, X::Value& retValue);
			~Device();
		private:
			std::string m_deviceId;
			std::unique_ptr<SerialPort> m_serialPort;
			std::mutex m_responseMutex;
			std::condition_variable m_responseCondition;
			int m_commandIndex;
			std::thread m_readThread;
			std::atomic<bool> m_running;

			std::unordered_map<int, std::shared_ptr<std::pair<X::Value, std::condition_variable>>> m_pendingCommands;

			inline std::vector<char> ConvertToBytes(X::Value& val);
			void ReadLoop();
		};
	}
}
