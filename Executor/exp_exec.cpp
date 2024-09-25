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

#include "exp_exec.h"
#include "xport.h"
#include "op.h"
#include "module.h"
#include "var.h"
#include "pair.h"
#include "list.h"
#include "import.h"
#include "exp_work.h"

/*
To improve exec spped, remove Exec from Expresion and dereived classses,
merged with ExpExec
we are going to do this way:
1) add an Expression Stack,which will hold expression to be executed
2) check stack top, if it is children havn't been pushed into stack, push them
method is run PreExec(...) for this top expression, inside PreExec will
do push operation,and/or some other operations
3) also need to keep a list of values for this expression, children will add value into
this list, as it's return values, and this expression will use these values to run PostExec(...)
3) if top expression's children have been pushed into stack, then run this expression's
PostExec, with values from children
 for performance, PreExec and PostExec are not virtual functions.
*/

namespace X
{
	namespace Exp
	{
		bool ExpExec(AST::Expression* root,
			XlangRuntime* rt,
			AST::ExecAction& action,
			XObj* pContext,
			Value& v,
			LValue* lValue)
		{
			ExpresionStack expStack;
			expStack.push({ root, false });
			//hold current values from Children,will be used by this Expression's PostExec
			ValueStack valueStack;
			while (!expStack.empty())
			{
				AST::Expression* currentExp = expStack.top().first;
				bool& bPushed = expStack.top().second;
				if (bPushed)
				{
					RunPostExec(rt, pContext, currentExp,expStack,valueStack);
				}
				else
				{
					auto act = RunPreExec(rt, pContext, currentExp, expStack, valueStack);
					bPushed = true;
					if (act == Action::ExpStackPop)
					{//this expression has been processed including its children, pop it
						expStack.pop();
					}
				}
			}
			return true;
		}
	}
}