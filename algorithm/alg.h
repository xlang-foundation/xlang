#pragma once
#include "singleton.h"
#include "xpackage.h"
#include "xlang.h"
#include "graph.h"

namespace X
{
	namespace Algorithm
	{
		class A :
			public Singleton<A>
		{
		public:
			BEGIN_PACKAGE(A)
				APISET().AddClass<0, XGraph>("graph");
			END_PACKAGE
		};
	}
}