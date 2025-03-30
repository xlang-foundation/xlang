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

#include "PyFunc.h"
#include <mutex>

class MGil {
    bool m_needsRelease;
    PyGILState_STATE m_state;
    // A static mutex to protect the critical section.
    static std::mutex s_mutex;
public:
    // Constructor optionally auto-locks the GIL.
    inline MGil(bool autoLock = true)
        : m_needsRelease(false), m_state(PyGILState_UNLOCKED)
    {
        if (autoLock) {
            Lock();
        }
    }

    // Destructor releases the GIL if it was acquired.
    inline ~MGil()
    {
        if (m_needsRelease) {
            std::lock_guard<std::mutex> lock(s_mutex);
            PyGILState_Release(m_state);
        }
    }

    // Lock the GIL if it is not already held by this thread.
    inline void Lock()
    {
        if (!m_needsRelease) {
            std::lock_guard<std::mutex> lock(s_mutex);
            // Now the check and ensure are atomic with respect to other threads using MGil.
            if (!PyGILState_Check()) {
                m_state = PyGILState_Ensure();
                m_needsRelease = true;
            }
        }
    }

    // Unlock the GIL if this instance acquired it.
    inline void Unlock()
    {
        if (m_needsRelease) {
            std::lock_guard<std::mutex> lock(s_mutex);
            PyGILState_Release(m_state);
            m_needsRelease = false;
        }
    }
};

