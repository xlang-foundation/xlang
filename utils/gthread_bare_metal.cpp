#include "gthread.h"

#if defined(BARE_METAL)
    bool GThread::Start(){
        return true;
    }
    void GThread::Stop(){
        
    }
    void GThread::WaitToEnd(){
        
    }

    bool GThread2::Start(){
        return true;
    }
    void GThread2::Stop(){
        
    }
    void GThread2::WaitToEnd(){
        
    }
#endif