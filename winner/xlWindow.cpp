#include "xlWindow.h"
#include <windows.h> 
#include "xlImage.h"
#include "xlApp.h"
#include "utility.h"

X::Value::operator XWin::Box* ()const
{
    if (x.obj->GetType() == ObjType::Package)
    {
        XPackage* pPack = dynamic_cast<XPackage*>(x.obj);
        return (XWin::Box*)pPack->GetEmbedObj();
    }
    else
    {
        return nullptr;
    }
}
X::Value::operator XWin::Window* ()const
{
    if (x.obj->GetType() == ObjType::Package)
    {
        XPackage* pPack = dynamic_cast<XPackage*>(x.obj);
        return (XWin::Window*)pPack->GetEmbedObj();
    }
    else
    {
        return nullptr;
    }
}
X::Value::operator XWin::Menu* ()const
{
    if (x.obj->GetType() == ObjType::Package)
    {
        XPackage* pPack = dynamic_cast<XPackage*>(x.obj);
        return (XWin::Menu*)pPack->GetEmbedObj();
    }
    else
    {
        return nullptr;
    }
}

namespace XWin
{
#define   XL_WIN_CLS_NAME "XL_WIN_CLASS"

    LBox::LBox(Window* parent)
    {
        if (parent)
        {
            parent->AddLayoutBox(this);
            m_parent = parent;
        }
    }
    ControlBase::ControlBase(Window* parent)
    {
        m_parent = parent;
        if (m_parent)
        {
            m_parent->AddLayoutBox(this);
        }
    }
    bool ControlBase::SetText(std::string text)
    {
        m_text = text;
        if (m_hwnd)
        {
            SetWindowText((HWND)m_hwnd, m_text.c_str());
        }
        return true;
    }
    void ControlBase::onAreaChanged()
    {
        if (m_hwnd)
        {
            MoveWindow((HWND)m_hwnd, m_rc.left, m_rc.top,
                m_rc.right - m_rc.left, m_rc.bottom - m_rc.top, TRUE);
        }
    }
    bool ControlBase::Show(bool visible)
    {
        if (m_hwnd == nullptr)
        {
            return false;
        }
        ShowWindow((HWND)m_hwnd, visible ? SW_SHOW : SW_HIDE);
        UpdateWindow((HWND)m_hwnd);
        return true;
    }
    bool ControlBase::Repaint()
    {
        if (m_hwnd == nullptr)
        {
            return false;
        }
        InvalidateRect((HWND)m_hwnd, NULL, TRUE);
        UpdateWindow((HWND)m_hwnd);
        return true;
    }
    bool Button::Create()
    {
        auto hinstance = GetModuleHandle(NULL);
        auto hwndButton = CreateWindow(
            "BUTTON",
            m_text.c_str(),
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            m_rc.left,         // x position 
            m_rc.top,         // y position 
            m_rc.right-m_rc.left,        // Button width
            m_rc.bottom-m_rc.top,        // Button height
            (HWND)m_parent->GetWnd(),     // Parent window
            NULL,       // No menu.
            hinstance,
            NULL);
        SetWindowLongPtr(hwndButton, GWLP_USERDATA, (LONG_PTR)this);
        m_hwnd = hwndButton;
        return true;
    }

    bool TextEditBox::Create()
    {
        auto hinstance = GetModuleHandle(NULL);
        auto hwndEdit = CreateWindowEx(
            0, "EDIT",   // predefined class 
            NULL,         // no window title 
            WS_CHILD | WS_VISIBLE | WS_VSCROLL |
            ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL,
            m_rc.left, m_rc.top, m_rc.right - m_rc.left, m_rc.bottom - m_rc.top,
            (HWND)m_parent->GetWnd(),         // parent window 
            (HMENU)0,   // edit control ID 
            hinstance,
            NULL);        // pointer not needed 
        SetWindowLongPtr(hwndEdit, GWLP_USERDATA, (LONG_PTR)this);
        m_hwnd = hwndEdit;
        return true;
    }
    LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        Window* pWindow = (Window*)GetWindowLongPtr(hWnd,GWLP_USERDATA);
        switch (message)
        {
        case WM_COMMAND:
        {
            HWND childWnd = (HWND)lParam;
            int wmId = LOWORD(wParam);
            int evt = HIWORD(wParam);
            if (evt == BN_CLICKED)
            {
                Button* pButton = (Button*)GetWindowLongPtr(childWnd, GWLP_USERDATA);
                X::ARGS params;
                X::KWARGS kwargs;
                if (pButton)
                {
                    pButton->Fire(0, params, kwargs);
                }
            }
            // Parse the menu selections:
            switch (wmId)
            {
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
        case WM_SIZE:
        {
            if (pWindow)
            {
                pWindow->OnSize();
                X::ARGS params;
                X::KWARGS kwargs;
                pWindow->Fire(1, params, kwargs);
            }
        }
        break;
        case WM_PAINT:
        {
            X::ARGS params;
            X::KWARGS kwargs;
#if 0
            pWindow->Fire(0, params, kwargs);
            ValidateRect(hWnd, NULL);
#else
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            pWindow->Fire(0, params, kwargs);
            EndPaint(hWnd, &ps);
#endif
        }
        break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        return 0;
    }
    BOOL InitApplication(HINSTANCE hinstance)
    {
        WNDCLASSEX wcx;

        // Fill in the window class structure with parameters 
        // that describe the main window. 

        wcx.cbSize = sizeof(wcx);          // size of structure 
        wcx.style = CS_HREDRAW |
            CS_VREDRAW;                    // redraw if size changes 
        wcx.lpfnWndProc = WndProc;     // points to window procedure 
        wcx.cbClsExtra = 0;                // no extra class memory 
        wcx.cbWndExtra = 0;                // no extra window memory 
        wcx.hInstance = hinstance;         // handle to instance 
        wcx.hIcon = LoadIcon(NULL,
            IDI_APPLICATION);              // predefined app. icon 
        wcx.hCursor = LoadCursor(NULL,
            IDC_ARROW);                    // predefined arrow 
        wcx.hbrBackground = (HBRUSH)GetStockObject(
            WHITE_BRUSH);                  // white background brush 
        wcx.lpszMenuName = nullptr;// "MainMenu";    // name of menu resource 
        wcx.lpszClassName = XL_WIN_CLS_NAME;  // name of window class 
        wcx.hIconSm = nullptr;
#if 0
        wcx.hIconSm = LoadImage(hinstance, // small class icon 
            MAKEINTRESOURCE(5),
            IMAGE_ICON,
            GetSystemMetrics(SM_CXSMICON),
            GetSystemMetrics(SM_CYSMICON),
            LR_DEFAULTCOLOR);
#endif
        // Register the window class. 

        return RegisterClassEx(&wcx);
    }
    void Window::OnSize()
    {
        for (auto* box : m_Boxes)
        {
            box->StartVisiting();
        }
        bool HaveMore = true;
        int loopNum = (int)m_Boxes.size();
        int loopNo = 0;
        while (HaveMore && (loopNo < loopNum))
        {
            HaveMore = false;
            for (auto* box : m_Boxes)
            {
                if (box->IsVisited())
                {
                    continue;
                }
                if (!box->HaveConstraint())
                {
                    box->EndVisiting();
                }
                Rect newRect = { -1,-1,-1,-1 };
                //Left
                Coord* pRefCoord = &box->GetCoordRect().left;
;               Box* pRefBox = pRefCoord->ancorBox;
                if (pRefBox)
                {
                    if (pRefBox->IsVisited())
                    {
                        newRect.left = pRefBox->GetSide(this,pRefCoord->side,pRefCoord->Offset);
                    }
                    else
                    {
                        HaveMore = true;
                        continue;
                    }
                }
                //Top
                pRefCoord = &box->GetCoordRect().top;
                pRefBox = pRefCoord->ancorBox;
                if (pRefBox)
                {
                    if (pRefBox->IsVisited())
                    {
                        newRect.top = pRefBox->GetSide(this, pRefCoord->side, pRefCoord->Offset);
                    }
                    else
                    {
                        HaveMore = true;
                        continue;
                    }
                }
                //Right
                pRefCoord = &box->GetCoordRect().right;
                pRefBox = pRefCoord->ancorBox;
                if (pRefBox)
                {
                    if (pRefBox->IsVisited())
                    {
                        newRect.right = pRefBox->GetSide(this, pRefCoord->side, pRefCoord->Offset);
                    }
                    else
                    {
                        HaveMore = true;
                        continue;
                    }
                }
                //Bottom
                pRefCoord = &box->GetCoordRect().bottom;
                pRefBox = pRefCoord->ancorBox;
                if (pRefBox)
                {
                    if (pRefBox->IsVisited())
                    {
                        newRect.bottom = pRefBox->GetSide(this, pRefCoord->side, pRefCoord->Offset);
                    }
                    else
                    {
                        HaveMore = true;
                        continue;
                    }
                }
                if (newRect.left != -1 || newRect.top != -1 
                    || newRect.right != -1 || newRect.bottom != -1)
                {//if at least one side set,then pass to this box
                    box->SetRect(newRect);
                }
            }
            loopNo++;
        }
    }
    bool Window::Create()
    {
        DWORD dwStyle = WS_OVERLAPPEDWINDOW;
        HWND hParent = NULL;
        int x = m_rc.left;
        int y = m_rc.top;
        int w = m_rc.right - m_rc.left;
        int h = m_rc.bottom - m_rc.top;
        if (x == 0 && y == 0 && w == 0 && h == 0)
        {
            x = y = w = h = CW_USEDEFAULT;
        }
        if (m_parent)
        {
            hParent = (HWND)m_parent->m_hwnd;
            if (hParent)
            {
                dwStyle = WS_CHILD | WS_VISIBLE;
            }
        }
        auto hinstance = GetModuleHandle(NULL);
        BOOL bOK = InitApplication(hinstance);
        DWORD error = GetLastError();
        auto hwnd = CreateWindow(
            XL_WIN_CLS_NAME,
            m_text.c_str(),
            dwStyle,x,y,w,h,
            hParent,
            (HMENU)NULL,
            hinstance,
            (LPVOID)this);
        error = GetLastError();
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)this);
        m_hwnd = hwnd;
        return true;
    }

    bool Toolbar::Create()
    {
        auto hinstance = GetModuleHandle(NULL);
        const int ImageListID = 0;
        const DWORD buttonStyles = BTNS_AUTOSIZE| BTNS_SHOWTEXT;
        // Create the toolbar.
        HWND hWndToolbar = CreateWindowEx(0, TOOLBARCLASSNAME, NULL,
            WS_CHILD | TBSTYLE_WRAPABLE| /*TBSTYLE_FLAT |*/ TBSTYLE_TOOLTIPS, 0, 0, 0, 0,
            (HWND)m_parent->GetWnd(), NULL, hinstance, NULL);
        if (hWndToolbar == NULL)
        {
            return false;
        }
        // Create the image list.
        m_hImageList = ImageList_Create(m_cx, m_cy,ILC_COLOR32 | ILC_MASK,m_numButtons, 0);

        // Set the image list.
        SendMessage(hWndToolbar, TB_SETIMAGELIST,(WPARAM)ImageListID,(LPARAM)m_hImageList);
        ImageList_Add(m_hImageList, m_pImgeList->ToHBITMAP(), nullptr);
        // Load the button images.
        //SendMessage(hWndToolbar, TB_LOADIMAGES,(WPARAM)IDB_STD_LARGE_COLOR,(LPARAM)HINST_COMMCTRL);

        std::vector<TBBUTTON> tbButtons;
        for (int i = 0; i < m_numButtons; i++)
        {
            int idCommand = WinManager::I().GetNewCommandId();
            ButtonInfo& info = m_ButtonInfos[i];
            tbButtons.push_back({ MAKELONG(i,  ImageListID), 
                idCommand,TBSTATE_ENABLED, 
                buttonStyles, {0}, 0, (INT_PTR)info.name.c_str() });
        }
        // Add buttons.
        SendMessage(hWndToolbar, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);
        SendMessage(hWndToolbar, TB_ADDBUTTONS, (WPARAM)m_numButtons, (LPARAM)tbButtons.data());

        // Resize the toolbar, and then show it.
        SendMessage(hWndToolbar, TB_AUTOSIZE, 0, 0);
        ShowWindow(hWndToolbar, TRUE);
        m_hwnd = hWndToolbar;
        return true;
    }
    X::Value Window::CreateChildWindow(int x, int y, int w, int h)
    {
        Window* pWin = new Window(this);
        pWin->SetRect(x, y, w, h);
        pWin->Create();
        return X::Value(APISET().GetProxy(pWin), false);
    }
    bool Menu::Insert(int idx, std::string txt)
    {
        MENUITEMINFO menuItemInfo;
        menuItemInfo.cbSize = sizeof(MENUITEMINFO);
        menuItemInfo.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_DATA | MIIM_STATE;
        menuItemInfo.fType = MFT_STRING;
        menuItemInfo.dwTypeData = (LPTSTR)txt.c_str();
        menuItemInfo.cch = (int)txt.size() + 1;
        menuItemInfo.dwItemData = (ULONG_PTR)WinManager::I().GetNewCommandId();
        menuItemInfo.fState = 0;

        InsertMenuItem(m_hMenu, idx, TRUE, &menuItemInfo);
        DWORD e = GetLastError();
        return true;
    }
    bool Menu::InsertSubMenu(int idx, Menu* pSubMenu, std::string txt)
    {
        InsertMenu(m_hMenu, idx,MF_POPUP, (UINT_PTR)pSubMenu->Get(), txt.c_str());
        return true;
    }
    bool Window::SetMenu(Menu* pMenu)
    {
        ::SetMenu((HWND)m_hwnd, pMenu->Get());
        return true;
    }
}