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

#include "xlApp.h"
#include <windows.h> 

namespace XWin
{

#define UI_RUN_MSGID  (WM_USER+1)
	bool UIThreadRun(X::Value& callable, void* pContext)
	{
		((App*)pContext)->AddUICall(callable);
		return true;
	}
	void App::PostMessage(unsigned int msgId, void* pParam)
	{
		PostThreadMessage(m_threadId, msgId, (WPARAM)pParam, NULL);
	}
	void App::AddUICall(X::Value& callable)
	{
		m_callLock.Lock();
		m_calls.push_back(callable);
		m_callLock.Unlock();
		PostMessage(UI_RUN_MSGID, NULL);
	}
	void App::Process()
	{
		while (true)
		{
			X::Value callable;
			m_callLock.Lock();
			if (m_calls.size() > 0)
			{
				callable = m_calls[0];
				m_calls.erase(m_calls.begin());
			}
			m_callLock.Unlock();
			if (callable.IsObject())
			{
				X::Value retVal;
				X::ARGS args(0);
				X::KWARGS kwargs;
				callable.GetObj()->Call(nullptr,nullptr,args, kwargs, retVal);
			}
			else
			{
				break;
			}
		}
	}
	bool App::Loop()
	{
		m_threadId = GetCurrentThreadId();
		X::g_pXHost->RegisterUIThreadRunHandler(UIThreadRun,this);
		MSG msg;
		BOOL fGotMessage;
		while ((fGotMessage = GetMessage(&msg, (HWND)NULL, 0, 0)) != 0 
			&& fGotMessage != -1)
		{
			if (msg.message == UI_RUN_MSGID)
			{
				Process();
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		return true;
	}
}