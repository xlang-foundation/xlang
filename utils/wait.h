#pragma once

typedef void* XWaitHandle;
class XWait
{
public:
	XWait(bool autoReset=true);
	~XWait();
	bool Wait(int timeoutMS);
	void Release();
private:
	bool m_autoReset = true;
	XWaitHandle m_waitHandle = nullptr;
};