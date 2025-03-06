#pragma once

#include "value.h"
#include <sstream> 
#include <string>
#include <iostream>
#include "xlog.h"

namespace X
{
	typedef int (*LOGLOCK)(); // return dump level
	typedef void (*LOGUNLOCK)();
	typedef void (*LOGWRITE)(int level, const char* log);

	class Log :
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

		virtual void operator<<(XlangLog& l) override
		{
			if (m_realLogger)
			{
				m_realLogger(m_level, "\n");
				if (m_unlock)
				{
					m_unlock();
				}
			}
			else
			{
				if (m_level <= m_dumpLevel)
				{
					std::cout << std::endl;
				}
			}
		}

		virtual void Write(const char* msg) override
		{
			if (m_realLogger)
			{
				m_realLogger(m_level, msg);
			}
			else if(m_level <= m_dumpLevel)
			{
				std::cout << msg;
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
			if (m_lock)
			{
				m_lock();
			}
		}

		inline void SetLevel(int l)
		{
			m_level = l;
		}
		inline void SetDumpLevel(int l)
		{
			m_dumpLevel = l;
		}
	private:
		int m_level = 0;
		int m_dumpLevel = 999999; // All levels will dump out
	};

	extern Log log;

#define InitLog(lock, unlock, logger) X::log.Init(lock, unlock, logger)
#define SetLogLevel(l) X::log.SetDumpLevel(l)
}
