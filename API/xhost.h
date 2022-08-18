#ifndef _X_HOST_H_
#define _X_HOST_H_

namespace X
{
	class XObj;
	class XHost
	{
	public:
		virtual XObj* CreateStrObj(const char* data, int size) = 0;
	};
	extern XHost* g_pXHost;
}

#endif