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
			ADD_FUNC("generate_uid", generate_uid)
		END_PACKAGE
		Utils()
		{
		}
		bool generate_uid(void* rt, XObj* pContext,
			ARGS& params,
			KWARGS& kwParams,
			X::Value& retValue)
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
			retValue = X::Value(strGuid);
			return true;
		}
	};
}