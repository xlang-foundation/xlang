#pragma once
#include "xpackage.h"
#include "xlang.h"

namespace XWin
{
	class Window;
	class Button
	{
		void* m_hwnd = nullptr;
		Window* m_parent = nullptr;
	public:
		BEGIN_PACKAGE(Button)
			APISET().AddEvent("Click");
			APISET().AddFunc<1>("Show", &Button::Show);
		END_PACKAGE
		Button(Window* parent,std::string caption);
		Button(Window* parent, std::string caption,int x,int y, int w,int h);
		bool Show(bool visible);
	};
	class TextEditBox
	{
		void* m_hwnd = nullptr;
		Window* m_parent = nullptr;
	public:
		BEGIN_PACKAGE(TextEditBox)
			APISET().AddEvent("Click");
			APISET().AddFunc<1>("SetText", &TextEditBox::SetText);
			APISET().AddFunc<1>("Show", &TextEditBox::Show);
		END_PACKAGE
		TextEditBox(Window* parent,int x, int y, int w, int h);
		bool SetText(std::string text);
		bool Show(bool visible);
	};
	class Window
	{
		void* m_hwnd = nullptr;
	public:
		void* GetWnd() { return m_hwnd; }
		BEGIN_PACKAGE(Window)
			APISET().AddClass<5, Button, Window>("Button");
			APISET().AddClass<4, TextEditBox, Window>("TextEditBox");
			APISET().AddEvent("OnDraw");
			APISET().AddFunc<1>("Show", &Window::Show);
		END_PACKAGE
		Window(std::string name);
		bool Show(bool visible);
	};

}