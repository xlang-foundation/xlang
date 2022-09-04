#include "dbg.h"
#include "utility.h"

namespace X
{
	bool Dbg::xTraceFunc(
		Runtime* rt,
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
			break;
		case X::TraceEvent::Exception:
			break;
		case X::TraceEvent::Line:
			bOK = Dbg(rt).Check(traceEvent,rt,pThisBlock,
				pCurrentObj, pContext);
			break;
		case X::TraceEvent::Return:
			bOK = Dbg(rt).ExitBlockRun(rt, pContext, 
				pThisBlock);
			break;
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
		Runtime* rt = (Runtime*)(unsigned long long)objRuntimeHandle;
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
			auto* pProxyScope = dynamic_cast<Data::PyProxyObject*>(lastScope);
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
		blockObj->AddRef();
		blockObj->SetPyFrame(objFrame);
		blockObj->SetModule(pProxyModule);
		blockObj->SetModuleFileName(fileName);
		blockObj->AddRef();//for pThisBlock
		pThisBlock = dynamic_cast<AST::Scope*>(blockObj);
		auto* pLine = new Data::PyProxyObject(line-1);//X start line from 0 not 1
		pLine->AddRef();
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
		if (pProxyModule)
		{
			pProxyModule->Release();
		}
		if (blockObj)
		{
			blockObj->Release();
		}
		if (pLine)
		{
			pLine->Release();
		}
		if (te == TraceEvent::Return)
		{
			rt->PopFrame();
			delete frameProxy;
		}
		return 0;
	}

}