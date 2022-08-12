#pragma once
#include "gthread.h"

namespace X {
	namespace Dev {
		class DevRouter:
			public GThread
		{
			// Inherited via GThread
			virtual void run() override;
		};
	}
}