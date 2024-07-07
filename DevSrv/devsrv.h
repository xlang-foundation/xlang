#pragma once

#include "httplib.h"
#include "gthread.h"

namespace X
{
	class DevServer :
		public GThread
	{
		httplib::Server m_srv;
		int m_port = 0;
		int m_padding = 0;
	public:
		DevServer(int port);
		~DevServer();

		// Inherited via GThread
		virtual void run() override;
	private:
		bool m_Connected = false;
	};
}