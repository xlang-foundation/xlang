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
		XPackageAPISet<Utils> m_Apis;
	public:
		XPackageAPISet<Utils>& APISET() { return m_Apis; }
		Utils()
		{
			m_Apis.AddFunc<0>("generate_uid", &Utils::generate_uid);
			m_Apis.Create(this);
		}
		X::Value generate_uid()
		{
			std::string strGuid;
#if (WIN32)
			GUID gid;
			CoCreateGuid(&gid);
			char szGuid[128];
			sprintf_s(szGuid, "%08X%04X%04X%X%X%X%X%X%X%X%X",
				gid.Data1, gid.Data2, gid.Data3,
				gid.Data4[0], gid.Data4[1], gid.Data4[2], gid.Data4[3],
				gid.Data4[4], gid.Data4[5], gid.Data4[6], gid.Data4[7]);
			strGuid = szGuid;
#else
			uuid_t uuid;
			uuid_generate_time_safe(uuid);
			char szGuid[128];
			uuid_unparse(uuid, szGuid);

			strGuid = szGuid;
#endif
			return X::Value(strGuid);
		}
	};
}