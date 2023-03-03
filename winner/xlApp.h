#pragma once
#include "singleton.h"
#include "xpackage.h"
#include "xlang.h"
#include "xlWindow.h"
#include "xlImage.h"
#include "Locker.h"

namespace XWin
{
	class App:
		public Singleton<App>
	{
		X::Value m_curModule;
		unsigned int m_threadId = 0;
		Locker m_callLock;
		std::vector<X::Value> m_calls;
	public:
		BEGIN_PACKAGE(App)
			APISET().AddFunc<0>("Loop", &App::Loop,"App.Loop()");
			APISET().AddClass<0, Window>("Window");
			APISET().AddClass<1, Image>("Image");
		END_PACKAGE
		void SetModule(X::Value curModule)
		{
			m_curModule = curModule;
		}
		void Process();
		void AddUICall(X::Value& callable);
		X::Value& GetModule() { return m_curModule; }
		void PostMessage(unsigned int msgId, void* pParam);
		bool Loop();
	};
}
