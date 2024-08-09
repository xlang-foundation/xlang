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
			void setReadCallback();
		};
	}
}
