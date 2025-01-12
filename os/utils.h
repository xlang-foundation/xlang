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
#if (WIN32)
#include <Windows.h>
#else
#include <uuid/uuid.h>
#endif

namespace X
{
	class Utils
	{
	public:
		BEGIN_PACKAGE(Utils)
			APISET().AddFunc<0>("generate_uid", &Utils::generate_uid);
		END_PACKAGE
		X::Value generate_uid()
		{
			std::string strGuid;
#if (WIN32)
			GUID gid;
			CoCreateGuid(&gid);
			char szGuid[128];
			sprintf_s(szGuid, "%08X%04X%04X%02X%02X%02X%02X%02X%02X%02X%02X",
				gid.Data1, gid.Data2, gid.Data3,
				gid.Data4[0], gid.Data4[1], gid.Data4[2], gid.Data4[3],
				gid.Data4[4], gid.Data4[5], gid.Data4[6], gid.Data4[7]);
			strGuid = szGuid;
#else
			uuid_t uuid;
#if defined(__linux__)
			uuid_generate_time_safe(uuid);
#else
			uuid_generate_time(uuid);
#endif
			char szGuid[128];
			uuid_unparse(uuid, szGuid);

			strGuid = szGuid;
#endif
			return X::Value(strGuid);
		}
	};
}