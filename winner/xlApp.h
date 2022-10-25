#pragma once
#include "singleton.h"
#include "xpackage.h"
#include "xlang.h"
#include "xlWindow.h"

namespace XWin
{
	class App:
		public Singleton<App>
	{
	public:
		BEGIN_PACKAGE(App)
			APISET().AddFunc<0>("Loop", &App::Loop);
			APISET().AddClass<1, Window>("Window");
		END_PACKAGE

		bool Loop();
	};
}
