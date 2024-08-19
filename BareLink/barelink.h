#pragma once
#include "xpackage.h"
#include "xlang.h"
#include <vector>
#include <string>
#include <iostream>
#include <regex>
#include "singleton.h"
#include "device.h"

namespace X
{
	namespace BareLink
	{
		class Manager:
			public Singleton<Manager>
		{
			//for calling XModule
			X::Value m_curModule;
			std::string m_curModulePath;
		public:
			BEGIN_PACKAGE(Manager)
				APISET().AddFunc<0>("EnumDevices", &Manager::EnumDevices);
				APISET().AddClass<1, Device>("Device");
			END_PACKAGE
				void SetModule(X::Value curModule)
			{
				X::XModule* pModule = dynamic_cast<X::XModule*>(curModule.GetObj());
				if (pModule)
				{
					auto path = pModule->GetPath();
					m_curModulePath = path;
					g_pXHost->ReleaseString(path);
				}
				m_curModule = curModule;
			}
			X::Value EnumDevices()
			{
				X::List devices;
				return devices;
			}
		};
	}
}