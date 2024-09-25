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

#include "DeviceLoop.h"
#include "xlstream.h"

DeviceLoop::DeviceLoop(uart_inst_t* uart, uint txPin, uint rxPin) :
	m_serialPort(uart, txPin, rxPin) {
	m_serialPort.configure(115200, 1000, 1000);
}

void DeviceLoop::start() {
		m_serialPort.asyncRead(DeviceLoop::onDataReceived, this);
}
void DeviceLoop::onDataReceived(const std::vector<char>& data, void* context) {
    DeviceLoop* self = static_cast<DeviceLoop*>(context);
    self->processCommand(data);
}
void DeviceLoop::processCommand(const std::vector<char>& command) {
	X::Value result;
	X::XLStream* pStream = X::g_pXHost->CreateStream(command.data(), command.size());
	X::Value params;
	params.FromBytes(pStream);
	X::g_pXHost->ReleaseStream(pStream);

	if (params.Size() < 2) {
		return; // Invalid command format
	}
	int commandIndex = (int)params[(int)0];
	auto commandType = (CommandType)(int)params[(int)1];
	switch (commandType) {
	case CommandType::LoadCode:
		if (params.Size() >= 4)
		{
			std::string moduleName = params[(int)2].ToString();
			std::string code = params[3].ToString();
			unsigned long long moduleKey = 0;
			LoadCode(moduleName, code, moduleKey);
			result = X::Value(moduleKey);
		}
		else
		{
			result = X::Value(0);
		}
		break;
	case CommandType::RunCode:
		if (params.Size() >= 3)
		{
			//pass in the module key and a flag: true for debug run, false for normal run
			unsigned long long moduleKey = (unsigned long long)params[(int)2];
			bool debug = false;
			if (params.Size() >= 4)
			{
				debug = (bool)params[3];
			}
			auto it = m_moduleMap.find(moduleKey);
			if (it != m_moduleMap.end())
			{
				X::Value objModule = it->second;
				int argNum = params.Size() - 4;
				X::ARGS args(argNum);
				for (int i = 0; i < argNum; i++)
				{
					args[i] = params[i + 4];
				}
				X::g_pXHost->RunModule(objModule, args, result,false);
			}
			else
			{
				result = X::Value();
			}
		}
		break;
	case CommandType::StopCodeRun:
		// Process StopCodeRun command with params
		//result = ...; // Set the result
		break;
	default:
		// Unknown command
		return;
	}
	X::List retList;
	retList += commandIndex;
	retList += result;
	X::Value retValue(retList);
	pStream = X::g_pXHost->CreateStream();
	retValue.ToBytes(pStream);
	int size = pStream->Size();
	std::vector<char> responseData(size);
	pStream->FullCopyTo(responseData.data(), size);
	X::g_pXHost->ReleaseStream(pStream);
	m_serialPort.asyncWrite(responseData); // Sending the result back
}

std::vector<char> DeviceLoop::ConvertToBytes(X::Value& val) {
	X::XLStream* pStream = X::g_pXHost->CreateStream();
	val.ToBytes(pStream);
	int size = pStream->Size();
	std::vector<char> data(size);
	pStream->FullCopyTo(data.data(), size);
	X::g_pXHost->ReleaseStream(pStream);
	return data;
}

bool DeviceLoop::LoadCode(std::string& moduleName, std::string& code, unsigned long long& moduleKey)
{
	X::Value objModule;
	bool bOK = X::g_pXHost->LoadModule(moduleName.c_str(), code.c_str(),
		code.size(), objModule);
	if (!bOK)
	{
		moduleKey = 0;
		return false;
	}
	auto* pObj = objModule.GetObj();
	moduleKey = (unsigned long long)pObj;
	m_moduleMap.emplace(std::make_pair(moduleKey, objModule));
	return true;
}
