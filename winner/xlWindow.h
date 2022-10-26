#pragma once
#include "xpackage.h"
#include "xlang.h"
#include "xlDraw.h"

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
			APISET().AddEvent("OnDraw");
			APISET().AddEvent("OnSize");
			APISET().AddClass<5, Button, Window>("Button");
			APISET().AddClass<0, Draw, Window>("Draw");
			APISET().AddClass<4, TextEditBox, Window>("TextEditBox");
			APISET().AddFunc<1>("Show", &Window::Show);
			APISET().AddFunc<0>("Repaint", &Window::Repaint);
			APISET().AddFunc<4>("CreateChildWindow", &Window::CreateChildWindow);
		END_PACKAGE
		Window(std::string name);
		Window(Window* pParent, int x, int y, int w, int h);
		X::Value CreateChildWindow(int x, int y, int w, int h);
		bool Repaint();
		bool Show(bool visible);
	};

}