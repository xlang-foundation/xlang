#pragma once
#include "singleton.h"
#include "xpackage.h"
#include "xlang.h"
#include "xlWindow.h"
#include "xlImage.h"
#include "Locker.h"

namespace XWin
{
	struct UIThreadRunParameter
	{
		X::Value context;
		X::Value callable;
		X::ARGS args;
		X::KWARGS kwParams;
	};
	class App:
		public Singleton<App>
	{
		X::Value m_curModule;
		unsigned int m_threadId = 0;
		Locker m_callLock;
		std::vector<UIThreadRunParameter> m_calls;
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
		void AddUICall(UIThreadRunParameter call);
		X::Value& GetModule() { return m_curModule; }
		void PostMessage(unsigned int msgId, void* pParam);
		bool Loop();
	};
}
