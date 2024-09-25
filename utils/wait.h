/*
Copyright (C) 2024 The XLang Foundation
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#pragma once

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

	template <typename Predicate>
	bool Wait(int timeoutMS, Predicate pred) {
		std::unique_lock<std::mutex> lock(m_mutex);
		bool predicateResult = pred();
		if (predicateResult || (m_signaled && predicateResult)) {
			if (m_autoReset && m_signaled)
				m_signaled = false;
			return true;
		}
		if (timeoutMS < 0)
			m_condition.wait(lock, [this, &pred] { return m_signaled && pred(); });
		else {
			if (!m_condition.wait_for(lock, std::chrono::milliseconds(timeoutMS),
				[this, &pred] { return m_signaled && pred(); }))
				return false;
		}
		if (m_autoReset && pred()) {
			m_signaled = false;
		}
		return true;
	}

	void Release(bool bAll = false) {
		std::lock_guard<std::mutex> lock(m_mutex);
		m_signaled = true;
		if (bAll) {
			m_condition.notify_all();
		}
		else {
			m_condition.notify_one();
		}
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
