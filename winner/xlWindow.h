#pragma once
#include "xpackage.h"
#include "xlang.h"
#include "xlDraw.h"

namespace XWin
{
	class Window;
	struct Rect
	{
		int left, top;
		int right, bottom;
	};
	class ControlBase
	{
	protected:
		void* m_hwnd = nullptr;
		Window* m_parent = nullptr;
		Rect m_rc={0,0,0,0};
		virtual void onAreaChanged();
	public:
		void* GetWnd() { return m_hwnd; }
		BEGIN_PACKAGE(ControlBase)
			APISET().AddPropL("Left",
				[](auto* pThis, X::Value v) {pThis->m_rc.left = v; pThis->onAreaChanged(); },
				[](auto* pThis) {return pThis->m_rc.left; });
			APISET().AddPropL("Top",
				[](auto* pThis, X::Value v) {pThis->m_rc.top = v; pThis->onAreaChanged(); },
				[](auto* pThis) {return pThis->m_rc.top; });
			APISET().AddPropL("Right",
				[](auto* pThis, X::Value v) {pThis->m_rc.right = v; pThis->onAreaChanged(); },
				[](auto* pThis) {return pThis->m_rc.right; });
			APISET().AddPropL("Bottom",
				[](auto* pThis, X::Value v) {pThis->m_rc.bottom = v; pThis->onAreaChanged(); },
				[](auto* pThis) {return pThis->m_rc.bottom; });
			APISET().AddFunc<1>("Show", &ControlBase::Show);
			APISET().AddFunc<0>("Repaint", &ControlBase::Repaint);
		END_PACKAGE
		bool Show(bool visible);
		bool Repaint();
	};
	class Button:
		public ControlBase
	{
	public:
		BEGIN_PACKAGE(Button)
			ADD_BASE(ControlBase);
			APISET().AddEvent("Click");
		END_PACKAGE
		Button(Window* parent,std::string caption);
		Button(Window* parent, std::string caption,int x,int y, int w,int h);
	};
	class TextEditBox:
		public ControlBase
	{
	public:
		BEGIN_PACKAGE(TextEditBox)
			ADD_BASE(ControlBase);
			APISET().AddEvent("Click");
			APISET().AddFunc<1>("SetText", &TextEditBox::SetText);
		END_PACKAGE
		TextEditBox(Window* parent,int x, int y, int w, int h);
		bool SetText(std::string text);
	};
	class Window :
		public ControlBase
	{
	public:
		BEGIN_PACKAGE(Window)
			ADD_BASE(ControlBase);
			APISET().AddEvent("OnDraw");
			APISET().AddEvent("OnSize");
			APISET().AddClass<5, Button, Window>("Button");
			APISET().AddClass<0, Draw, Window>("Draw");
			APISET().AddClass<4, TextEditBox, Window>("TextEditBox");
			APISET().AddFunc<4>("CreateChildWindow", &Window::CreateChildWindow);
		END_PACKAGE
		Window(std::string name);
		Window(Window* pParent, int x, int y, int w, int h);
		X::Value CreateChildWindow(int x, int y, int w, int h);
	};

}