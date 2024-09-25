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

#include <iostream>
#include <string>
#include "xpackage.h"
#include "xlang.h"

namespace X {

	class Service {
		// Export the Service class and its functions in the API:
		BEGIN_PACKAGE(Service)
			APISET().AddPropWithType<std::string>("ServiceName", &Service::serviceName);
			APISET().AddFunc<2>("CreateService", &Service::CreateService);
			APISET().AddFunc<0>("StartService", &Service::StartService);
			APISET().AddFunc<0>("StopService", &Service::StopService);
			APISET().AddFunc<0>("RemoveService", &Service::RemoveService);
		END_PACKAGE

	public:
		Service(const std::string& name) : serviceName(name) {}

		bool CreateService(const std::string& binaryPath, const std::string& startType) {
#if (WIN32)
			SC_HANDLE scManager = OpenSCManager(nullptr, nullptr, SC_MANAGER_CREATE_SERVICE);
			if (!scManager) {
				std::cerr << "OpenSCManager failed: " << GetLastError() << std::endl;
				return false;
			}

			SC_HANDLE scService = ::CreateService(
				scManager,
				serviceName.c_str(),
				serviceName.c_str(),
				SERVICE_ALL_ACCESS,
				SERVICE_WIN32_OWN_PROCESS,
				GetStartType(startType),
				SERVICE_ERROR_NORMAL,
				binaryPath.c_str(),
				nullptr, nullptr, nullptr, nullptr, nullptr);

			if (!scService) {
				std::cerr << "CreateService failed: " << GetLastError() << std::endl;
				CloseServiceHandle(scManager);
				return false;
			}

			CloseServiceHandle(scService);
			CloseServiceHandle(scManager);
			return true;
#else
			// Linux implementation would go here
			return false;
#endif
		}

		bool StartService() {
#if (WIN32)
			SC_HANDLE scManager = OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT);
			if (!scManager) {
				std::cerr << "OpenSCManager failed: " << GetLastError() << std::endl;
				return false;
			}

			SC_HANDLE scService = ::OpenService(scManager, serviceName.c_str(), SERVICE_START);
			if (!scService) {
				std::cerr << "OpenService failed: " << GetLastError() << std::endl;
				CloseServiceHandle(scManager);
				return false;
			}

			if (!::StartService(scService, 0, nullptr)) {
				std::cerr << "StartService failed: " << GetLastError() << std::endl;
				CloseServiceHandle(scService);
				CloseServiceHandle(scManager);
				return false;
			}

			CloseServiceHandle(scService);
			CloseServiceHandle(scManager);
			return true;
#else
			// Linux implementation would go here
			return false;
#endif
		}

		bool StopService() {
#if (WIN32)
			SC_HANDLE scManager = OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT);
			if (!scManager) {
				std::cerr << "OpenSCManager failed: " << GetLastError() << std::endl;
				return false;
			}

			SC_HANDLE scService = ::OpenService(scManager, serviceName.c_str(), SERVICE_STOP);
			if (!scService) {
				std::cerr << "OpenService failed: " << GetLastError() << std::endl;
				CloseServiceHandle(scManager);
				return false;
			}

			SERVICE_STATUS status;
			if (!::ControlService(scService, SERVICE_CONTROL_STOP, &status)) {
				std::cerr << "ControlService failed: " << GetLastError() << std::endl;
				CloseServiceHandle(scService);
				CloseServiceHandle(scManager);
				return false;
			}

			CloseServiceHandle(scService);
			CloseServiceHandle(scManager);
			return true;
#else
			// Linux implementation would go here
			return false;
#endif
		}

		bool RemoveService() {
#if (WIN32)
			SC_HANDLE scManager = OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT);
			if (!scManager) {
				std::cerr << "OpenSCManager failed: " << GetLastError() << std::endl;
				return false;
			}

			SC_HANDLE scService = ::OpenService(scManager, serviceName.c_str(), DELETE);
			if (!scService) {
				std::cerr << "OpenService failed: " << GetLastError() << std::endl;
				CloseServiceHandle(scManager);
				return false;
			}

			if (!::DeleteService(scService)) {
				std::cerr << "DeleteService failed: " << GetLastError() << std::endl;
				CloseServiceHandle(scService);
				CloseServiceHandle(scManager);
				return false;
			}

			CloseServiceHandle(scService);
			CloseServiceHandle(scManager);
			return true;
#else
			// Linux implementation would go here
			return false;
#endif
		}

	private:
		std::string serviceName;

#if (WIN32)
		DWORD GetStartType(const std::string& startType) {
			if (startType == "auto") {
				return SERVICE_AUTO_START;
			}
			else if (startType == "manual") {
				return SERVICE_DEMAND_START;
			}
			else {
				return SERVICE_DISABLED;
			}
		}
#endif
	};

} //
