#pragma once
#include "gthread.h"
#include "event.h"
#include "singleton.h"

namespace X 
{
	class EventLoopThread :
		public Singleton<EventLoopThread>,
		public GThread
	{
		// Inherited via GThread
		virtual void run() override
		{
			EventSystem::I().Loop();
		}
	};
}