#pragma once

//#define USE_MYWAIT

#if defined(USE_MYWAIT)
typedef void* XWaitHandle;
class XWait
{
public:
    XWait(bool autoReset = true);
    ~XWait();
    bool Wait(int timeoutMS);
    void Reset();
    void Release();
private:
    bool m_autoReset = true;
    XWaitHandle m_waitHandle = nullptr;
};
#else
#include <condition_variable>
#include <mutex>
#include <chrono>

class XWait {
public:
	XWait(bool autoReset = true) : m_autoReset(autoReset), m_signaled(false) {}

	~XWait() = default;

	bool Wait(int timeoutMS) {
		std::unique_lock<std::mutex> lock(m_mutex);
		if (m_signaled)
		{
			if (m_autoReset)
				m_signaled = false;
			return true;
		}
		if (timeoutMS < 0)
			m_condition.wait(lock, [this] { return m_signaled; });
		else {
			if (!m_condition.wait_for(lock, std::chrono::milliseconds(timeoutMS), [this] { return m_signaled; }))
				return false;
		}
		if (m_autoReset) {
			m_signaled = false;
		}
		return true;
	}

	void Release() {
		std::lock_guard<std::mutex> lock(m_mutex);
		m_signaled = true;
		m_condition.notify_one();
	}

	void Reset() {
		std::lock_guard<std::mutex> lock(m_mutex);
		m_signaled = false;
	}

private:
	bool m_autoReset;
	bool m_signaled;
	std::mutex m_mutex;
	std::condition_variable m_condition;
};

#endif