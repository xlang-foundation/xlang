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
#include <mutex>
#include <condition_variable>

namespace X
{
	namespace IPC
	{
        class CallCounter {
        private:
            std::mutex m_mutex;
            std::condition_variable m_cv;
            int m_count = 0;

        public:
            void AddCall() {
                std::lock_guard<std::mutex> lock(m_mutex);
                ++m_count;
            }

            void RemoveCall() {
                std::lock_guard<std::mutex> lock(m_mutex);
                if (--m_count == 0) {
                    m_cv.notify_all();
                }
            }

            void WaitForZero() {
                std::unique_lock<std::mutex> lock(m_mutex);
                m_cv.wait(lock, [this]() { return m_count == 0; });
            }

            int GetCount() {
                std::lock_guard<std::mutex> lock(m_mutex);
                return m_count;
            }
        };
        class AutoCallCounter {
        private:
            CallCounter& m_counter;

        public:
            AutoCallCounter(CallCounter& counter) : m_counter(counter) {
                m_counter.AddCall();
            }

            ~AutoCallCounter() {
                m_counter.RemoveCall();
            }
        };
	}
}