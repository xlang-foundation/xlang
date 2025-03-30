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

    // A static mutex to protect the critical section
    static std::mutex s_mutex;

    // Thread-local counter to track nested locks
    static thread_local int s_lockCounter;

public:
    // Constructor optionally auto-locks the GIL
    inline MGil(bool autoLock = true)
        : m_needsRelease(false), m_state(PyGILState_UNLOCKED)
    {
        if (autoLock) {
            Lock();
        }
    }

    // Destructor releases the GIL if this instance acquired it
    inline ~MGil()
    {
        Unlock();
    }

    // Lock the GIL if it is not already held by this thread
    inline void Lock()
    {
        std::lock_guard<std::mutex> lock(s_mutex);

        // If this is the first lock in this thread, acquire the GIL
        if (s_lockCounter == 0 && !PyGILState_Check()) {
            // PyGILState_Ensure will automatically initialize the thread state if needed
            m_state = PyGILState_Ensure();
            m_needsRelease = true;
        }

        // Increment the lock counter
        s_lockCounter++;
    }

    // Unlock the GIL if this is the outermost MGil instance
    inline void Unlock()
    {
        if (m_needsRelease) {
            std::lock_guard<std::mutex> lock(s_mutex);

            // Decrement the lock counter
            s_lockCounter--;

            // Only release the GIL if this is the outermost lock
            if (s_lockCounter == 0) {
                PyGILState_Release(m_state);
                m_needsRelease = false;
            }
        }
    }
};
