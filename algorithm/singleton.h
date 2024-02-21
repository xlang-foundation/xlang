#pragma once

#if !defined(FORCE_INLINE)
#if defined(_MSC_VER)
// Microsoft Visual C++ Compiler
#define FORCE_INLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
// GCC or Clang Compiler
#define FORCE_INLINE __attribute__((always_inline)) inline
#else
// Fallback for other compilers
#define FORCE_INLINE inline
#endif
#endif

template <class T> 
class Singleton { 
public: 
	FORCE_INLINE static T& I() { 
		static T _instance; 
		return _instance; 
	} 

protected: 
	Singleton(void) {} 
	virtual ~Singleton(void) {} 
	Singleton(const Singleton<T>&); 
	Singleton<T>& operator= (const Singleton<T> &); 
}; 