#include "log.h"
#include "port.h"
#include "utility.h"

X::Log X::log;

X::Log::Log():m_lock(nullptr), m_unlock(nullptr), m_realLogger(nullptr)
{
}

X::Log::~Log()
{

}

X::Log& X::Log::SetCurInfo(const char* fileName,
	const int line, const int level)
{
	if (m_lock == nullptr || m_unlock == nullptr || m_realLogger == nullptr)
	{
		return *this;
	}
	int dumpLevel = m_lock();
	m_level = level;
	if (m_level <= dumpLevel)
	{
		std::string strFileName(fileName);
		auto pos = strFileName.rfind(Path_Sep_S);
		if (pos != std::string::npos)
		{
			strFileName = strFileName.substr(pos + 1);
		}
		unsigned long pid = GetPID();
		unsigned long tid = GetThreadID();
		int64_t ts = getCurMicroTimeStamp();

		const int buf_Len = 1000;
		char szFilter[buf_Len];
		SPRINTF(szFilter, buf_Len, "[%d-%d-%llu,%s:%d] ", pid, tid, ts, strFileName.c_str(), line);
		m_realLogger(m_level,szFilter);
	}
	return *this;
}