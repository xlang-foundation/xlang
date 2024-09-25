/*
Copyright (C) 2024 The XLang Foundation
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

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
