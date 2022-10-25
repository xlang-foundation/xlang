#include "xlApp.h"
#include <windows.h> 

namespace XWin
{
	bool App::Loop()
	{
		MSG msg;
		BOOL fGotMessage;
		while ((fGotMessage = GetMessage(&msg, (HWND)NULL, 0, 0)) != 0 
			&& fGotMessage != -1)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		return true;
	}
}