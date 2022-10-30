#pragma once
#include "singleton.h"
#include "xpackage.h"
#include "xlang.h"
#include "xlDraw.h"
#include <Windows.h>
#include <commctrl.h>

namespace XWin
{
	class Window;
	class Image;
	struct Rect
	{
		int left, top;
		int right, bottom;
	};
	class WinManager :
		public Singleton<WinManager>
	{
		int m_lastCommandId = 10000;
	public:
		int GetNewCommandId() { return m_lastCommandId++; }
	};
	class ControlBase
	{
	protected:
		void* m_hwnd = nullptr;
		Window* m_parent = nullptr;
		Rect m_rc={0,0,0,0};
		std::string m_text;
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
			APISET().AddFunc<4>("SetRect", &ControlBase::SetRect);
			APISET().AddFunc<1>("Show", &ControlBase::Show);
			APISET().AddFunc<0>("Repaint", &ControlBase::Repaint);
			APISET().AddFunc<1>("SetText", &ControlBase::SetText);
			APISET().AddFunc<0>("Create", &ControlBase::Create);
		END_PACKAGE
		ControlBase(Window* parent)
		{
			m_parent = parent;
		}
		virtual bool Create() = 0;
		bool SetRect(int x, int y, int w, int h)
		{
			m_rc = { x,y,x + w,y + h };
			onAreaChanged();
			return true;
		}
		bool SetText(std::string text);
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
		Button(Window* parent) :
			ControlBase(parent)
		{

		}
		bool Create();
	};
	class Toolbar :
		public ControlBase
	{
		struct ButtonInfo
		{
			std::string name;
		};
		HIMAGELIST m_hImageList = nullptr;
		Image* m_pImgeList = nullptr;
		int m_numButtons = 0;
		int m_cx = 16;
		int m_cy = 16;
		std::vector<ButtonInfo> m_ButtonInfos;
	public:
		BEGIN_PACKAGE(Toolbar)
			ADD_BASE(ControlBase);
			APISET().AddFunc<4>("SetImageList", &Toolbar::SetImageList);
			APISET().AddFunc<2>("SetButtonText", &Toolbar::SetButtonText);
		END_PACKAGE
		Toolbar(Window* parent) :
			ControlBase(parent)
		{
		}
		bool Create();
		bool SetImageList(Image* pImg, int numButtons, int cx, int cy)
		{
			m_pImgeList = pImg;
			m_numButtons = numButtons;
			m_cx = cx;
			m_cy = cy;
			m_ButtonInfos.resize(m_numButtons);
			return true;
		}
		bool SetButtonText(int idx, std::string text)
		{
			if (idx >= m_ButtonInfos.size())
			{
				return false;
			}
			m_ButtonInfos[idx].name = text;
			return true;
		}
	};
	class TextEditBox:
		public ControlBase
	{
	public:
		BEGIN_PACKAGE(TextEditBox)
			ADD_BASE(ControlBase);
			APISET().AddEvent("Click");
		END_PACKAGE
		TextEditBox(Window* parent) :
			ControlBase(parent)
		{
		}
		bool Create();
	};
	class Menu
	{
		HMENU m_hMenu = nullptr;
		BEGIN_PACKAGE(Menu)
			APISET().AddFunc<2>("Insert", &Menu::Insert);
			APISET().AddFunc<3>("InsertSubMenu", &Menu::InsertSubMenu);
		END_PACKAGE
		Menu()
		{
			m_hMenu = CreateMenu();
		}
		HMENU Get() { return m_hMenu; }
		bool Insert(int idx, std::string txt);
		bool InsertSubMenu(int idx,Menu* pSubMenu, std::string txt);
	};
	class Window :
		public ControlBase
	{
	public:
		BEGIN_PACKAGE(Window)
			ADD_BASE(ControlBase);
			APISET().AddEvent("OnDraw");
			APISET().AddEvent("OnSize");
			APISET().AddClass<0, Menu>("Menu");
			APISET().AddClass<0, Toolbar, Window>("Toolbar");
			APISET().AddClass<0, Button, Window>("Button");
			APISET().AddClass<0, Draw, Window>("Draw");
			APISET().AddClass<0, TextEditBox, Window>("TextEditBox");
			APISET().AddFunc<4>("CreateChildWindow", &Window::CreateChildWindow);
			APISET().AddFunc<1>("SetMenu", &Window::SetMenu);
			END_PACKAGE
		Window(Window* parent):ControlBase(parent)
		{
		}
		Window() :ControlBase(nullptr)
		{
		}
		bool SetMenu(Menu* pMenu);
		X::Value CreateChildWindow(int x, int y, int w, int h);
		bool Create();
	};

}