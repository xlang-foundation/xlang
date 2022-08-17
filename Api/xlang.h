#ifndef _X_LANG_H_
#define _X_LANG_H_

#include <string>

namespace X
{
	class XObj
	{
	public:
		virtual int IncRef() = 0;
		virtual int DecRef() = 0;
		virtual std::string GetTypeString() = 0;
		virtual long long Size() = 0;
		virtual size_t Hash() = 0;
		virtual std::string ToString(bool WithFormat = false)=0;
	};
	class XFunc:
		virtual public XObj
	{

	};
}

#endif