#pragma once
#include "gthread.h"

namespace X {
	namespace Dev {
		class DevRouter:
			public GThread
		{
			bool m_run;
			// Inherited via GThread
			virtual void run() override;
		};
	}
}