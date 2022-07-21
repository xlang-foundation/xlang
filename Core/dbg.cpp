#include "dbg.h"
#include "utility.h"

namespace X
{
	bool Dbg::xTraceFunc(
		Runtime* rt,
		void* pContext,
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
		PyEng::Object objRuntimeHandle(self, true);
		Runtime* rt = (Runtime*)(unsigned long long)objRuntimeHandle;
		PyEng::Object objFrame(frame, true);
		auto fileName = (std::string)objFrame["f_code.co_filename"];
		auto coName = (std::string)objFrame["f_code.co_name"];
		int line = (int)objFrame["f_lineno"];
		TraceEvent te = (TraceEvent)event;
		auto* pProxyModule = Data::PyObjectCache::I().QueryModule(fileName);
		AST::Scope* pThisBlock = nullptr;
		Data::PyProxyObject* blockObj = nullptr;
		if ("<module>" == coName)
		{
			pThisBlock = dynamic_cast<AST::Scope*>(pProxyModule);
		}
		else
		{
			blockObj = new Data::PyProxyObject(coName);
			blockObj->AddRef();
			blockObj->SetModule(pProxyModule);
			blockObj->SetModuleFileName(fileName);
			blockObj->AddRef();//for pThisBlock
			pThisBlock = dynamic_cast<AST::Scope*>(blockObj);
		}
		auto* pLine = new Data::PyProxyObject(line-1);//X start line from 0 not 1
		pLine->AddRef();
		pLine->SetScope(pThisBlock);
		Data::PyStackFrame* frameProxy = nullptr;
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
		if (blockObj)
		{
			blockObj->SetLocGlob(PyEng::Object::FromLocals(),
				PyEng::Object::FromGlobals());
		}
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
#if __todo_remove_
		PyEng::Object objFrame(frame, true);
		auto fileName = (std::string)objFrame["f_code.co_filename"];
		int line = (int)objFrame["f_lineno"];

		switch (te)
		{
		case TraceEvent::Call:
			std::cout << "---Call----" << std::endl;
			break;
		case TraceEvent::Exception:
			std::cout << "---Exception----" << std::endl;
			break;
		case TraceEvent::Line:
			//Dbg(rt).PyWaitForCommnd(rt);
			std::cout << "---Line----" << line << "," << fileName << std::endl;
			break;
		case TraceEvent::Return:
			std::cout << "---Return----" << std::endl;
			break;
		case TraceEvent::C_Call:
			std::cout << "---C_Call----" << std::endl;
			break;
		case TraceEvent::C_Exception:
			std::cout << "---C_Exception----" << std::endl;
			break;
		case TraceEvent::C_Return:
			std::cout << "---C_Return----" << std::endl;
			break;
		case TraceEvent::OPCode:
			std::cout << "---OPCode----" << std::endl;
			break;
		default:
			std::cout << "---Other----" << std::endl;
			break;
		}
		return 0;
#endif
	}
	void Dbg::PyWaitForCommnd(Runtime* rt)
	{
		auto* pModule = rt->M();
		AST::CommandInfo cmdInfo;
		bool inLoop = true;
		while (inLoop)
		{
			pModule->PopCommand(cmdInfo);
			switch (cmdInfo.dbgType)
			{
			case AST::dbg::Continue:
				inLoop = false;
				break;
			case AST::dbg::Step:
				inLoop = false;
				break;
			default:
				break;
			}
		}
	}

}