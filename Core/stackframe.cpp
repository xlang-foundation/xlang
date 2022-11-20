#include "stackframe.h"
#include "glob.h"

namespace X {
	namespace AST {
#if XLANG_ENG_DBG
		void StackFrame::ObjDbgSet(XObj* pObj)
		{
			G::I().ObjBindToStack(pObj, this);
		}
		void StackFrame::ObjDbgRemove(XObj* pObj)
		{
			G::I().ObjUnbindToStack(pObj, this);
		}
#endif
	}
}