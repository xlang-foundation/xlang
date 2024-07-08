#ifndef Locker_H
#define Locker_H

#if defined(WIN32)
#include <Windows.h>
#elif defined(BARE_METAL)
// Bare metal does not support mutexes, using empty implementations
#else
#include <mutex>
#endif

#if !defined(FORCE_INLINE)
#if defined(_MSC_VER)
// Microsoft Visual C++ Compiler
#define FORCE_INLINE __forceinline
#elif defined(BARE_METAL)
#define FORCE_INLINE 
#elif defined(__GNUC__) || defined(__clang__)
// GCC or Clang Compiler
#define FORCE_INLINE __attribute__((always_inline)) inline
#else
// Fallback for other compilers
#define FORCE_INLINE inline
#endif
#endif

class Locker {
    void* m_cs = nullptr;
public:
    FORCE_INLINE Locker() {
#if defined(WIN32)
        auto* cs = new CRITICAL_SECTION();
        InitializeCriticalSection(cs);
        m_cs = (void*)cs;
#elif defined(BARE_METAL)
        // No mutex implementation for bare metal, use empty initialization
#else
        m_cs = (void*)new std::recursive_mutex();
#endif
    }

    FORCE_INLINE ~Locker() {
#if defined(WIN32)
        DeleteCriticalSection((CRITICAL_SECTION*)m_cs);
        delete (CRITICAL_SECTION*)m_cs;
#elif defined(BARE_METAL)
        // No mutex implementation for bare metal, use empty deletion
#else
        delete (std::recursive_mutex*)m_cs;
#endif
    }

    FORCE_INLINE void Lock() {
#if defined(WIN32)
        ::EnterCriticalSection((CRITICAL_SECTION*)m_cs);
#elif defined(BARE_METAL)
        // No mutex implementation for bare metal, use empty lock
#else
        ((std::recursive_mutex*)m_cs)->lock();
#endif
    }

    FORCE_INLINE void Unlock() {
#if defined(WIN32)
        ::LeaveCriticalSection((CRITICAL_SECTION*)m_cs);
#elif defined(BARE_METAL)
        // No mutex implementation for bare metal, use empty unlock
#else
        ((std::recursive_mutex*)m_cs)->unlock();
#endif
    }
};

class AutoLock {
public:
    FORCE_INLINE AutoLock(Locker& lk) {
        m_lock = &lk;
        lk.Lock();
    }

    FORCE_INLINE ~AutoLock() {
        if (m_lock) {
            m_lock->Unlock();
        }
    }

private:
    Locker* m_lock;
};

#endif // Locker_H
