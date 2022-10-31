#pragma once
#include "singleton.h"
#include "xpackage.h"
#include "xlang.h"
#include "xlWindow.h"
#include "xlImage.h"

namespace XWin
{
	class App:
		public Singleton<App>
	{
		X::Value m_curModule;
	public:
		BEGIN_PACKAGE(App)
			APISET().AddFunc<0>("Loop", &App::Loop);
			APISET().AddClass<0, Window>("Window");
			APISET().AddClass<1, Image>("Image");
		END_PACKAGE
		void SetModule(X::Value curModule)
		{
			m_curModule = curModule;
		}
		X::Value& GetModule() { return m_curModule; }
		bool Loop();
	};
}
