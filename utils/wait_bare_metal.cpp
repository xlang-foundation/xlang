#include "wait.h"
#if defined(BARE_METAL)
    XWait::XWait(bool autoReset=true){

    }
	XWait::~XWait(){

    }
	bool XWait::Wait(int timeoutMS){
        return true;
    }
	void XWait::Reset(){
        
    }
	void XWait::Release(){
        
    }
#endif

