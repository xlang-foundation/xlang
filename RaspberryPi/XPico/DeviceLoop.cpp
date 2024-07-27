#include "DeviceLoop.h"

DeviceLoop::DeviceLoop(uart_inst_t* uart, uint txPin, uint rxPin) : 
    m_serialPort(uart, txPin, rxPin) {
    m_serialPort.configure(115200, 1000, 1000);
}

void DeviceLoop::start() {
    while (true) {
        m_serialPort.asyncRead([this](const std::vector<char>& command) {
            processCommand(command);
            });
    }
}

void DeviceLoop::processCommand(const std::vector<char>& command) {
    X::Value result;
    X::XLStream* pStream = X::g_pXHost->CreateStream(command.data(), command.size());
    X::Value commandParams;
    commandParams.FromBytes(pStream);
    X::g_pXHost->ReleaseStream(pStream);

    X::List params(commandParams);
    if (params.size() < 2) {
        return; // Invalid command format
    }
    int commandIndex = (int)params[0];
    auto commandType = (X::BareLink::CommandType)(int)params[1];
    switch (commandType) {
    case X::BareLink::LoadCode:
        // Process LoadCode command with params
        //result = ...; // Set the result
        break;
    case X::BareLink::RunCode:
        // Process RunCode command with params
        //result = ...; // Set the result
        break;
    case X::BareLink::StopCodeRun:
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
