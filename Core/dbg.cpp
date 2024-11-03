﻿/*
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

#include "dbg.h"
#include "utility.h"
#include "op.h"
#include "dotop.h"

namespace X
{
	//XLang Trance Function

	bool Dbg::xTraceFunc(
		XlangRuntime* rt,
		XObj* pContext,
		AST::StackFrame* frame,
		TraceEvent traceEvent,
		AST::Scope* pThisBlock,
		AST::Expression* pCurrentObj)
	{
		bool bOK = true;
		switch (traceEvent)
		{
		case X::TraceEvent::Call:
			//for call, we don't need to do anything here
			break;
		case X::TraceEvent::Exception:
			break;
		case X::TraceEvent::Line:
			bOK = Dbg(rt).Check(traceEvent,rt,pThisBlock,
				pCurrentObj, pContext);
			break;
		/*case X::TraceEvent::Return:
			bOK = Dbg(rt).ExitBlockRun(rt, pContext, pThisBlock);
			break;*/
		case X::TraceEvent::C_Call:
			break;
		case X::TraceEvent::C_Exception:
			break;
		case X::TraceEvent::C_Return:
			break;
		case X::TraceEvent::OPCode:
			break;
		default:
			break;
		}
		return bOK;

	}
	int Dbg::PythonTraceFunc(
		PyEngObjectPtr self,
		PyEngObjectPtr frame,
		int event,
		PyEngObjectPtr args)
	{
		//first, quickly return as possible
		PyEng::Object objFrame(frame, true);
		auto fileName = (std::string)objFrame["f_code.co_filename"];
		if (fileName[0] == '<' && fileName[fileName.size() - 1] == '>')
		{
			return 0;
		}

		PyEng::Object objRuntimeHandle(self, true);
		XlangRuntime* rt = (XlangRuntime*)(unsigned long long)objRuntimeHandle;
		TraceEvent te = (TraceEvent)event;
		if (te == TraceEvent::Call)
		{//check if there is a scope which was from Command StepIn
			auto st = rt->M()->HaveWaitForScope();
			if(st == AST::ScopeWaitingStatus::NoWaiting)
			{
				return 0;
			}
		}
		else
		{//check if there is a scope match with this frame
			PyEng::Object objRuntimeHandle(self, true);
			AST::Scope* lastScope = rt->M()->LastScope();
			if (lastScope == nullptr)
			{
				return 0;
			}
			Data::PyProxyObject* pProxyScope = nullptr;
#if __TODO_SCOPE__
			pProxyScope = dynamic_cast<Data::PyProxyObject*>(lastScope);
#endif
			if (pProxyScope == nullptr)
			{
				return 0;
			}
			if (!pProxyScope->MatchPyFrame(frame))
			{
				return 0;
			}
		}

		Data::PyStackFrame* frameProxy = nullptr;
		auto coName = (std::string)objFrame["f_code.co_name"];
		int line = (int)objFrame["f_lineno"];
		auto* pProxyModule = Data::PyObjectCache::I().QueryModule(fileName);
		AST::Scope* pThisBlock = nullptr;
		auto* blockObj = new Data::PyProxyObject(coName);
		//blockObj->Scope::IncRef();
		blockObj->SetPyFrame(objFrame);
		blockObj->SetModule(pProxyModule);
		blockObj->SetModuleFileName(fileName);
		//blockObj->Scope::IncRef();//for pThisBlock
		pThisBlock = dynamic_cast<AST::Scope*>(blockObj);
		auto* pLine = new Data::PyProxyObject(line-1);//X start line from 0 not 1
		//pLine->Scope::IncRef();
		pLine->SetModuleFileName(fileName);
		pLine->SetScope(pThisBlock);
		if (te == TraceEvent::Call)
		{
			frameProxy = new Data::PyStackFrame(frame, pThisBlock);
			rt->PushFrame(frameProxy,0);
			rt->M()->ReplaceLastDbgScope(pThisBlock);
		}
		else
		{
			frameProxy = (Data::PyStackFrame*)rt->GetCurrentStack();
		}
		blockObj->SetLocGlob(PyEng::Object::FromLocals(),
			PyEng::Object::FromGlobals());
		frameProxy->SetLine(line);
		xTraceFunc(rt, nullptr, frameProxy,
			te, pThisBlock,
			dynamic_cast<AST::Expression*>(pLine));
#if __TODO_SCOPE__
		if (pProxyModule)
		{
			pProxyModule->Scope::DecRef();
		}
		if (blockObj)
		{
			blockObj->Scope::DecRef();
		}
		if (pLine)
		{
			pLine->Scope::DecRef();
		}
#endif
		if (te == TraceEvent::Return)
		{
			rt->PopFrame();
			delete frameProxy;
		}
		return 0;
	}

	bool Dbg::CalcCallables(AST::Expression* curExp, 
		std::vector<AST::Expression*>& callables)
	{
		if (curExp == nullptr)
		{
			return false;
		}
		switch (curExp->m_type)
		{
			case AST::ObType::Pair:
			{
				AST::PairOp* pPair = dynamic_cast<AST::PairOp*>(curExp);
				if (pPair)
				{
					callables.push_back(pPair);
					return true;
				}
			}
			break;
			default:
			{
				AST::BinaryOp* pBinOp = dynamic_cast<AST::BinaryOp*>(curExp);
				if (pBinOp)
				{
					return CalcCallables(pBinOp->GetL(), callables)
						|| CalcCallables(pBinOp->GetR(), callables);
				}
			}
		}
		return false;
	}

}