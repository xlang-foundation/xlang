#ifndef Locker_H
#define Locker_H

class Locker
{
	void* m_cs = nullptr;
public:
	Locker();
	~Locker();

	void Lock();
	void Unlock();
};

class AutoLock
{
public:
	AutoLock() :
		m_lock(nullptr)
	{
	}

	AutoLock(Locker& lk)
	{
		m_lock = &lk;
		lk.Lock();
	}

	~AutoLock()
	{
		if (m_lock)
		{
			m_lock->Unlock();
		}
	}

private:
	Locker* m_lock;
};

#endif //Locker_H
