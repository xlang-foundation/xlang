#include "xlApp.h"
#include <windows.h> 

namespace XWin
{

#define UI_RUN_MSGID  (WM_USER+1)
	bool UIThreadRun(X::Value& context, X::Value& callable, X::ARGS& args, X::KWARGS& kwParams)
	{
		App::I().AddUICall(UIThreadRunParameter{ context,callable,args,kwParams });
		return true;
	}
	void App::PostMessage(unsigned int msgId, void* pParam)
	{
		PostThreadMessage(m_threadId, msgId, (WPARAM)pParam, NULL);
	}
	void App::AddUICall(UIThreadRunParameter call)
	{
		m_callLock.Lock();
		m_calls.push_back(call);
		m_callLock.Unlock();
		PostMessage(UI_RUN_MSGID, NULL);
	}
	void App::Process()
	{
		while (true)
		{
			bool bGotCall = false;
			UIThreadRunParameter call;
			m_callLock.Lock();
			if (m_calls.size() > 0)
			{
				call = m_calls[0];
				m_calls.erase(m_calls.begin());
				bGotCall = true;
			}
			m_callLock.Unlock();
			if (bGotCall)
			{
				if (call.callable.IsObject())
				{
					X::Value retVal;
					call.callable.GetObj()->Call(nullptr, call.context.GetObj(), call.args, call.kwParams, retVal);
				}
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
		X::g_pXHost->RegisterUIThreadRunHandler(UIThreadRun);
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