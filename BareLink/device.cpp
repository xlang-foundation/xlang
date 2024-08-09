#include "device.h"
#include <chrono>
#include "xlstream.h"

namespace X
{
	namespace BareLink
	{
		Device::Device(std::string deviceId) : m_deviceId(deviceId), m_serialPort(nullptr), m_commandIndex(0), m_running(false) {}

		bool Device::Connect()
		{
			m_serialPort = std::make_unique<SerialPort>(m_deviceId.c_str());
			if (m_serialPort->open()) {
				m_serialPort->configure(115200, 1000, 1000);
				setReadCallback();
				m_serialPort->run();
				m_running = true;
				//m_readThread = std::thread(&Device::ReadLoop, this);
				return true;
			}
			else {
				m_serialPort.reset();
				return false;
			}
		}

		bool Device::Disconnect()
		{
			if (m_serialPort)
			{
				m_running = false;
				if (m_readThread.joinable()) {
					m_readThread.join();
				}
				m_serialPort->close();
				m_serialPort.reset();
			}
			return true;
		}

		bool Device::RunCommand(
			X::XRuntime* rt, X::XObj* pContext,
			X::ARGS& params, X::KWARGS& kwParams, X::Value& retValue)
		{
			if (params.size() < 2)
			{
				return false;
			}
			CommandType type =(CommandType)(int)params[0];
			int commandIndex = m_commandIndex++;

			X::List list;
			list += commandIndex;
			list += type;
			for (int i = 1; i < params.size(); i++)
			{
				list += params[i];
			}
			auto  it = kwParams.find("timeoutMs");
			int timeoutMs = 100000000;
			if (it)
			{
				timeoutMs = (int)it->val;
			}
			std::vector<char> data = ConvertToBytes(list);

			auto commandPair = std::make_shared<std::pair<X::Value, std::condition_variable>>();
			{
				std::lock_guard<std::mutex> lock(m_responseMutex);
				m_pendingCommands.emplace(std::make_pair(commandIndex,commandPair));
			}


			m_serialPort->asyncWrite(data);

			std::unique_lock<std::mutex> lock(m_responseMutex);
			if (!commandPair->second.wait_for(lock, std::chrono::milliseconds(timeoutMs), [&] {
				return m_pendingCommands[commandIndex]->first.IsValid();
				}))
			{
				std::lock_guard<std::mutex> lock(m_responseMutex);
				m_pendingCommands.erase(commandIndex);
			}

			X::Value response = m_pendingCommands[commandIndex]->first;
			{
				std::lock_guard<std::mutex> lock(m_responseMutex);
				m_pendingCommands.erase(commandIndex);
			}

			return response;
		}

		void Device::setReadCallback()
		{
			m_serialPort->setReadCallback([this](const std::vector<char>& data) {
				X::XLStream* pStream = X::g_pXHost->CreateStream(data.data(), data.size());
				X::Value returns;
				returns.FromBytes(pStream);
				X::g_pXHost->ReleaseStream(pStream);
				X::List listReturns(returns);
				if (listReturns->Size() < 2)
				{
					return;
				}
				int index = (int)listReturns->Get(0);
				X::Value retData = listReturns->Get(1);
				if (index >= 0) {
					std::unique_lock<std::mutex> lock(m_responseMutex);
					if (m_pendingCommands.find(index) != m_pendingCommands.end()) {
						m_pendingCommands[index]->first = retData;
						m_pendingCommands[index]->second.notify_one();
					}
				}
				else {
					//TODO: FireEvent(retData);
				}
				});
		}


		std::vector<char> Device::ConvertToBytes(X::Value& val)
		{
			X::XLStream* pStream = X::g_pXHost->CreateStream();
			val.ToBytes(pStream);
			int size = pStream->Size();
			std::vector<char> data(size);
			pStream->FullCopyTo(data.data(), size);
			X::g_pXHost->ReleaseStream(pStream);
			return data;
		}

		Device::~Device()
		{
			Disconnect();
		}
	}
}
