#ifndef __gthread_h__
#define __gthread_h__

class GThread
{
public:
    GThread()
    {

    }
    virtual ~GThread()
    {

    }
    virtual bool Start();
    virtual void Stop();
    virtual void WaitToEnd();
public:
    virtual void run() =0;
protected:
    void* mThreadHandle =nullptr;
};

#endif //!__gthread_h__