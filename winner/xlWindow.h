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
	class Box;
	class LBox;
	struct Rect
	{
		int left, top;
		int right, bottom;
	};
	enum class BoxSide
	{
		Left,Top,Right,Bottom
	};
	struct Coord
	{
		Box* ancorBox = nullptr;//ref pointer to Box,not own this Box
		BoxSide side = BoxSide::Left;
		int Offset = 0;
	};
	struct CoordRect
	{
		Coord left, top;
		Coord right, bottom;
	};
	class Box
	{
		bool m_visited = true;
	protected:
		CoordRect m_coordRect;
		BEGIN_PACKAGE(Box)
			APISET().AddFunc<3>("SetLeft", &Box::SetLeft);
			APISET().AddFunc<3>("SetTop", &Box::SetTop);
			APISET().AddFunc<3>("SetRight", &Box::SetRight);
			APISET().AddFunc<3>("SetBottom", &Box::SetBottom);
		END_PACKAGE
	public:
		CoordRect& GetCoordRect() { return m_coordRect; }
		void StartVisiting()
		{
			m_visited = false;
		}
		void EndVisiting()
		{
			m_visited = true;
		}
		bool HaveConstraint()
		{
			return m_coordRect.left.ancorBox != nullptr ||
				m_coordRect.top.ancorBox != nullptr ||
				m_coordRect.right.ancorBox != nullptr ||
				m_coordRect.bottom.ancorBox != nullptr;
		}
		void SetRect(Rect rc)
		{
			Set(rc);
			m_visited = true;
		}
		int GetSide(Box* pCurContainer,BoxSide side,int offset)
		{
			Rect rc = GetContainRect(pCurContainer);
			switch (side)
			{
			case XWin::BoxSide::Left:
				return rc.left + offset;
			case XWin::BoxSide::Top:
				return rc.top + offset;
			case XWin::BoxSide::Right:
				return rc.right + offset;
			case XWin::BoxSide::Bottom:
				return rc.bottom + offset;
			default:
				break;
			}
			return 0;
		}
		bool IsVisited() { return m_visited; }
		bool SetLeft(Box* pAncor, int side, int offset)
		{
			m_coordRect.left = Coord{ pAncor,(BoxSide)side,offset };
			return true;
		}
		bool SetTop(Box* pAncor, int side, int offset)
		{
			m_coordRect.top = Coord{ pAncor,(BoxSide)side,offset };
			return true;
		}
		bool SetRight(Box* pAncor, int side, int offset)
		{
			m_coordRect.right = Coord{ pAncor,(BoxSide)side,offset };
			return true;
		}
		bool SetBottom(Box* pAncor, int side, int offset)
		{
			m_coordRect.bottom = Coord{ pAncor,(BoxSide)side,offset };
			return true;
		}
		virtual Rect Get() = 0;
		virtual Rect GetContainRect(Box* pCurContainer) = 0;
		virtual void Set(Rect rc) = 0;
	};
	class WinManager :
		public Singleton<WinManager>
	{
		int m_lastCommandId = 10000;
	public:
		int GetNewCommandId() { return m_lastCommandId++; }
	};
	//Windowless Box for layout
	class LBox :
		public Box
	{
		Window* m_parent = nullptr;
		Rect m_rc = { 0,0,0,0 };
		BEGIN_PACKAGE(LBox)
			ADD_BASE(Box);
		END_PACKAGE
	public:
		LBox(Window* parent);
		virtual Rect GetContainRect(Box* pCurContainer) override
		{
			return m_rc;//for windowless, same as m_rc
		}
		virtual Rect Get() override
		{
			return m_rc;
		}
		virtual void Set(Rect rc)
		{
			m_rc = rc;
		}
	};
	class ControlBase:
		public Box
	{
	protected:
		void* m_hwnd = nullptr;
		Window* m_parent = nullptr;
		Rect m_rc={0,0,0,0};
		std::string m_text;
		virtual void onAreaChanged();
		virtual Rect GetContainRect(Box* pCurContainer) override
		{
			return m_rc;
		}
		virtual Rect Get() override
		{
			return m_rc;
		}
		virtual void Set(Rect rc)
		{
			if (rc.left != -1)
			{
				if (rc.right == -1)
				{
					m_rc.right = (m_rc.right - m_rc.left) + rc.left;
				}
				m_rc .left= rc.left;
			}
			if (rc.top != -1)
			{
				if (rc.bottom == -1)
				{
					m_rc.bottom = (m_rc.bottom - m_rc.top) + rc.top;
				}
				m_rc.top = rc.top;
			}
			if (rc.right != -1)
			{
				if (rc.left == -1)
				{
					m_rc.left = rc.right - (m_rc.right - m_rc.left);
				}
				m_rc.right = rc.right;
			}
			if (rc.bottom != -1)
			{
				if (rc.top == -1)
				{
					m_rc.top = rc.bottom - (m_rc.bottom - m_rc.top);
				}
				m_rc.bottom = rc.bottom;
			}
			MoveWindow((HWND)m_hwnd, m_rc.left, m_rc.top,
				m_rc.right - m_rc.left, m_rc.bottom - m_rc.top, TRUE);
			InvalidateRect((HWND)m_hwnd, nullptr, TRUE);
		}
	public:
		void* GetWnd() { return m_hwnd; }
		BEGIN_PACKAGE(ControlBase)
			ADD_BASE(Box);
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
		ControlBase(Window* parent);
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
		//for layout
		std::vector<Box*> m_Boxes;
	public:
		BEGIN_PACKAGE(Window)
			ADD_BASE(ControlBase);
			APISET().AddEvent("OnDraw");
			APISET().AddEvent("OnSize");
			APISET().AddClass<0, LBox, Window>("Box");
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
		virtual Rect GetContainRect(Box* pCurContainer) override
		{
			Rect rc;
			if (pCurContainer == this)
			{
				RECT rect;
				GetClientRect((HWND)m_hwnd, &rect);
				rc = { rect.left,rect.top,rect.right,rect.bottom };
			}
			else
			{
				rc = Get();
			}
			return rc;
		}
		void AddLayoutBox(Box* p)
		{
			m_Boxes.push_back(p);
		}
		void OnSize();
		bool SetMenu(Menu* pMenu);
		X::Value CreateChildWindow(int x, int y, int w, int h);
		bool Create();
	};

}