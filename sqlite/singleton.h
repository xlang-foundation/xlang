#pragma once


template <class T> 
class Singleton { 
public: 
	inline static T& I() { 
		static T _instance; 
		return _instance; 
	} 

protected: 
	Singleton(void) {} 
	virtual ~Singleton(void) {} 
	Singleton(const Singleton<T>&); 
	Singleton<T>& operator= (const Singleton<T> &); 
}; 