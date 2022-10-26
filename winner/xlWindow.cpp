#include "xlWindow.h"
#include <windows.h> 

namespace XWin
{
#define   XL_WIN_CLS_NAME "XL_WIN_CLASS"

    Button::Button(Window* parent,std::string caption)
    {
        auto hinstance = GetModuleHandle(NULL);
        auto hwndButton = CreateWindow(
            "BUTTON",  // Predefined class; Unicode assumed 
            caption.c_str(),      // Button text 
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles 
            10,         // x position 
            10,         // y position 
            100,        // Button width
            100,        // Button height
            (HWND)parent->GetWnd(),     // Parent window
            NULL,       // No menu.
            hinstance,
            NULL);
        SetWindowLongPtr(hwndButton, GWLP_USERDATA, (LONG_PTR)this);
        m_hwnd = hwndButton;
    }
    Button::Button(Window* parent, std::string caption, int x, int y, int w, int h)
    {
        auto hinstance = GetModuleHandle(NULL);
        auto hwndButton = CreateWindow(
            "BUTTON",  // Predefined class; Unicode assumed 
            caption.c_str(),      // Button text 
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles 
            x,         // x position 
            y,         // y position 
            w,        // Button width
            h,        // Button height
            (HWND)parent->GetWnd(),     // Parent window
            NULL,       // No menu.
            hinstance,
            NULL);
        SetWindowLongPtr(hwndButton, GWLP_USERDATA, (LONG_PTR)this);
        m_hwnd = hwndButton;
    }
    bool Button::Show(bool visible)
    {
        if (m_hwnd == nullptr)
        {
            return false;
        }
        ShowWindow((HWND)m_hwnd, visible ? SW_SHOW : SW_HIDE);
        UpdateWindow((HWND)m_hwnd);
        return true;
    }
    TextEditBox::TextEditBox(Window* parent,int x, int y, int w, int h)
    {
        auto hinstance = GetModuleHandle(NULL);
        auto hwndEdit = CreateWindowEx(
            0, "EDIT",   // predefined class 
            NULL,         // no window title 
            WS_CHILD | WS_VISIBLE | WS_VSCROLL |
            ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL,
            x, y, w, h, 
            (HWND)parent->GetWnd(),         // parent window 
            (HMENU)0,   // edit control ID 
            hinstance,
            NULL);        // pointer not needed 
        SetWindowLongPtr(hwndEdit, GWLP_USERDATA, (LONG_PTR)this);
        m_hwnd = hwndEdit;
    }
    bool TextEditBox::SetText(std::string text)
    {
        SetWindowText((HWND)m_hwnd, text.c_str());
        return true;
    }
    bool TextEditBox::Show(bool visible)
    {
        if (m_hwnd == nullptr)
        {
            return false;
        }
        ShowWindow((HWND)m_hwnd, visible ? SW_SHOW : SW_HIDE);
        UpdateWindow((HWND)m_hwnd);
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
                pButton->Fire(0, params, kwargs);
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
#if 1
    Window::Window(Window* pParent, int x, int y, int w, int h)
    {
        DWORD dwStyle = WS_CHILD | WS_VISIBLE;;
        HWND hParent = NULL;
        if (pParent)
        {
            hParent = (HWND)pParent->m_hwnd;
        }
        auto hinstance = GetModuleHandle(NULL);
        BOOL bOK = InitApplication(hinstance);
        DWORD error = GetLastError();
        auto hwnd = CreateWindow(
            XL_WIN_CLS_NAME,
            "",
            dwStyle,
            x,
            y,
            w,
            h,
            hParent,
            (HMENU)NULL,
            hinstance,
            (LPVOID)this);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)this);
        m_hwnd = hwnd;
    }
#endif
	Window::Window(std::string name)
	{
        DWORD dwStyle = WS_OVERLAPPEDWINDOW;
        auto hinstance = GetModuleHandle(NULL);
        BOOL bOK = InitApplication(hinstance);
        DWORD error = GetLastError();
        auto hwnd = CreateWindow(
            XL_WIN_CLS_NAME,
            name.c_str(),
            dwStyle,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            (HWND)NULL,
            (HMENU)NULL,
            hinstance,
            (LPVOID)this);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)this);
        m_hwnd = hwnd;
	}
    bool Window::Show(bool visible)
    {
        if (m_hwnd == nullptr)
        {
            return false;
        }
        ShowWindow((HWND)m_hwnd, visible?SW_SHOW:SW_HIDE);
        UpdateWindow((HWND)m_hwnd);
        return true;
    }
    bool Window::Repaint()
    {
        if (m_hwnd == nullptr)
        {
            return false;
        }
        InvalidateRect((HWND)m_hwnd, NULL, TRUE);
        UpdateWindow((HWND)m_hwnd);
        return true;
    }
    X::Value Window::CreateChildWindow(int x, int y, int w, int h)
    {
        Window* pWin = new Window(this,x,y,w,h);
        return X::Value(APISET().GetProxy(pWin), false);
    }
}