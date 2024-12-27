#pragma once
#include "xhost.h"
#include <sstream> 
#include <string>

namespace X
{
	class XlangLog
	{
	public:
		template<typename T>
		inline XlangLog& operator<<(const T& v)
		{
			std::ostringstream oss;
			oss << v;
			std::string message = oss.str();
			Write(message.c_str());
			return (XlangLog&)*this;
		}
		virtual void Write(const char* msg) = 0;
		virtual void operator<<(XlangLog& l) = 0;
		virtual XlangLog& SetCurInfo(const char* fileName, const int line, const int level) = 0;
		virtual XlangLog& LineEnd() = 0;
		virtual XlangLog& End() = 0;
	};

#define LOGV(level) ((XlangLog*)X::g_pXHost->GetLogger())->SetCurInfo(__FILE__,__LINE__,level)
#define LOG LOGV(0)
#define LOG1 LOGV(1)
#define LOG2 LOGV(2)
#define LOG3 LOGV(3)
#define LOG4 LOGV(4)
#define LOG5 LOGV(5)
#define LOG6 LOGV(6)
#define LOG7 LOGV(7)
#define LOG8 LOGV(8)
#define LOG9 LOGV(9)
#define LINE_END ((XlangLog*)X::g_pXHost->GetLogger())->LineEnd()
#define LOG_END ((XlangLog*)X::g_pXHost->GetLogger())->End()

// ANSI color codes for console
#define LOG_BLINK "\033[5m" //Blinking Text
#define LOG_RED "\033[31m"   // Red color
#define LOG_GREEN "\033[32m" // Green color
#define LOG_YELLOW "\033[33m" // Yellow color
#define LOG_BLUE "\033[34m"  // Blue color
#define LOG_RESET "\033[0m"  // Reset to default
}