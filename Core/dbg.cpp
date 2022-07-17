#include "dbg.h"
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
		case X::TraceEvent::Import:
			if (rt->M()->GetLastRequestDgbType() == X::AST::dbg::StepIn)
			{
				Dbg(rt).WaitForCommnd(
					traceEvent, rt, pThisBlock, pCurrentObj, pContext);
			}
			break;
		default:
			break;
		}
		return bOK;
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
		int line = (int)objFrame["f_lineno"];
		TraceEvent te = (TraceEvent)event;
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
	}
}