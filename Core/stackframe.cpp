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

#include "stackframe.h"
#include "glob.h"
#include "scope.h"

namespace X {
	namespace AST {
		bool StackFrame::AddVar(XlangRuntime* rt,std::string& name, X::Value& val)
		{
			AutoLock lock(m_lock);
			SCOPE_FAST_CALL_AddOrGet0(idx,m_pScope,name, false);
			Set(idx, val);
			return true;
		}
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