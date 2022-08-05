#include <windows.h>
#include <iostream>

extern void MainLoop();

#if (WIN32)
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    MainLoop();
    return 0;
}
#else
int main(int argc, char* argv[])
{
    MainLoop();
	return 0;
}
#endif