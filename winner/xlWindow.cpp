#include "xlWindow.h"
#include <windows.h> 

namespace XWin
{
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
        case WM_PAINT:
        {
            X::ARGS params;
            X::KWARGS kwargs;
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            pWindow->Fire(0, params, kwargs);
            auto hPen = CreatePen(PS_SOLID, 1, RGB(255,0, 0));
            SelectObject(hdc, hPen);
            SetBkColor(hdc, RGB(0, 0, 0));
            Rectangle(hdc, 10, 10, 200, 200);
            DeleteObject(hPen);
            EndPaint(hWnd, &ps);
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
        wcx.lpszClassName = "MainWClass";  // name of window class 
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
	Window::Window(std::string name)
	{
        auto hinstance = GetModuleHandle(NULL);
        BOOL bOK = InitApplication(hinstance);
        DWORD error = GetLastError();
        auto hwnd = CreateWindow(
            "MainWClass",       
            name.c_str(),
            WS_OVERLAPPEDWINDOW,
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
}