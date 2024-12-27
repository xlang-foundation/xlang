#pragma once

#include "value.h"
#include <sstream> 
#include <string>
#include "xlog.h"

namespace X
{
	typedef int (*LOGLOCK)(); //return dump level
	typedef void (*LOGUNLOCK)();
	typedef void (*LOGWRITE)(int level,const char* log);

	class Log:
		public XlangLog
	{
		LOGLOCK m_lock;
		LOGUNLOCK m_unlock;
		LOGWRITE m_realLogger;
	public:
		Log();
		~Log();

		void Init(LOGLOCK lock, LOGUNLOCK unlock, LOGWRITE log)
		{
			m_lock = lock;
			m_unlock = unlock;
			m_realLogger = log;
		}
		void GetLogFuncs(void** lock, void** unlock, void** logWrite)
		{
			*lock = m_lock;
			*unlock = m_unlock;
			*logWrite = m_realLogger;
		}

		virtual void operator<<(XlangLog& l) override
		{
			if (m_lock == nullptr || m_unlock == nullptr || m_realLogger == nullptr)
			{
				return;
			}
			m_realLogger(m_level, "\n");
			m_unlock();
		}
		virtual void Write(const char* msg) override
		{
			if (m_realLogger)
			{
				m_realLogger(m_level, msg);
			}
		}
		virtual Log& SetCurInfo(const char* fileName, const int line, const int level) override;
		virtual Log& LineEnd() override
		{
			return *this;
		}
		virtual Log& End() override
		{
			return *this;
		}
		inline void LineBegin()
		{
			if (m_lock == nullptr || m_unlock == nullptr || m_realLogger == nullptr)
			{
				return;
			}
			m_lock();
		}
		inline void SetLevel(int l)
		{
			m_level = l;
		}
	private:
		int m_level = 0;
	};
	extern Log log;
	#define InitLog(lock,unlock,logger) X::log.Init(lock,unlock,logger)
}