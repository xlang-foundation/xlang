#include "exp_exec.h"
#include "xport.h"
#include "op.h"
#include "module.h"
#include "var.h"
#include "pair.h"
#include "list.h"
#include "import.h"


namespace X
{
	namespace Exp
	{
		FORCE_INLINE Action RunPreExec(XlangRuntime* rt, XObj* pContext, AST::Expression* pExp,
			ExpresionStack& expStack, ValueStack& valueStack)
		{
			Action act = Action::Continue;
			auto expType = pExp->m_type;
			switch (expType)
			{
			case X::AST::ObType::Base:
				break;
			case X::AST::ObType::InlineComment:
				break;
			case X::AST::ObType::Assign:
				static_cast<X::AST::Assign*>(pExp)->Expanding(expStack);
				break;
			case X::AST::ObType::BinaryOp:
				static_cast<X::AST::BinaryOp*>(pExp)->Expanding(expStack);
				break;
			case X::AST::ObType::UnaryOp:
				static_cast<X::AST::UnaryOp*>(pExp)->Expanding(expStack);
				break;
			case X::AST::ObType::PipeOp:
				break;
			case X::AST::ObType::In:
				static_cast<X::AST::InOp*>(pExp)->Expanding(expStack);
				break;
			case X::AST::ObType::Var:
			{
				X::AST::ExecAction exeAction;
				ExpValue expValue = { pExp };
				auto ok = static_cast<X::AST::Var*>(pExp)->Exec(rt, exeAction, nullptr, expValue.v, &expValue.lv);
				valueStack.push(expValue);
				act = Action::ExpStackPop;
			}
			break;
			case X::AST::ObType::Str:
			{
				X::AST::ExecAction exeAction;
				ExpValue expValue = { pExp };
				auto ok = static_cast<X::AST::Str*>(pExp)->Exec(rt, exeAction, nullptr, expValue.v, &expValue.lv);
				valueStack.push(expValue);
				act = Action::ExpStackPop;
			}
			break;
			case X::AST::ObType::Const:
				break;
			case X::AST::ObType::Number:
			{
				auto val = static_cast<X::AST::Number*>(pExp)->GetValue();
				valueStack.push({ nullptr,val,nullptr });
				act = Action::ExpStackPop;
			}
			break;
			case X::AST::ObType::Double:
				break;
			case X::AST::ObType::Param:
				break;
			case X::AST::ObType::List:
			{
				auto& list = static_cast<X::AST::List*>(pExp)->GetList();
				for (auto* p : list)
				{
					expStack.push({ p,false });
				}
			}
			break;
			case X::AST::ObType::Pair:
				static_cast<X::AST::PairOp*>(pExp)->Expanding(expStack);
				break;
			case X::AST::ObType::Dot:
				static_cast<X::AST::DotOp*>(pExp)->Expanding(expStack);
				break;
			case X::AST::ObType::Decor:
				break;
			case X::AST::ObType::Func:
				break;
			case X::AST::ObType::JitBlock:
				break;
			case X::AST::ObType::BuiltinFunc:
				break;
			case X::AST::ObType::Module:
			{
				static_cast<X::AST::Module*>(pExp)->Expanding(expStack);
			}
			break;
			case X::AST::ObType::Block:
				break;
			case X::AST::ObType::Class:
				break;
			case X::AST::ObType::ReturnType:
				break;
			case X::AST::ObType::From:
				static_cast<X::AST::From*>(pExp)->Expanding(expStack);
				break;
			case X::AST::ObType::ColonOp:
				break;
			case X::AST::ObType::CommaOp:
				break;
			case X::AST::ObType::SemicolonOp:
				break;
			case X::AST::ObType::FeedOp:
				break;
			case X::AST::ObType::ActionOp:
				break;
			case X::AST::ObType::As:
				break;
			case X::AST::ObType::For:
			{
				auto* r = static_cast<X::AST::For*>(pExp)->GetR();
				expStack.push({ r,false });
			}
			break;
			case X::AST::ObType::While:
				break;
			case X::AST::ObType::If:
				break;
			case X::AST::ObType::ExternDecl:
				break;
			case X::AST::ObType::AwaitOp:
				break;
			case X::AST::ObType::Thru:
				static_cast<X::AST::ThruOp*>(pExp)->Expanding(expStack);
				break;
			case X::AST::ObType::Deferred:
				break;
			case X::AST::ObType::Import:
				static_cast<X::AST::Import*>(pExp)->Expanding(expStack);
				break;
			case X::AST::ObType::NamespaceVar:
				break;
			default:
				break;
			}
			return act;
		}
		FORCE_INLINE bool RunPostExec(XlangRuntime* rt, XObj* pContext, AST::Expression* pExp,
			ExpresionStack& expStack, ValueStack& valueStack)
		{
			bool needPopExpStack = true;
			auto expType = pExp->m_type;
			switch (expType)
			{
			case X::AST::ObType::InlineComment:
				break;
			case X::AST::ObType::Assign:
			{
				auto& left = valueStack.top();
				valueStack.pop();
				auto& right = valueStack.top();
				valueStack.pop();
				static_cast<X::AST::Assign*>(pExp)->ExpRun(rt, left, right);
			}
			break;
			case X::AST::ObType::BinaryOp:
			{
				auto& left = valueStack.top();
				valueStack.pop();
				auto& right = valueStack.top();
				valueStack.pop();
				X::Value retVal;
				auto ok = static_cast<X::AST::BinaryOp*>(pExp)->ExpRun(rt, left, right, retVal);
				valueStack.push({ pExp,retVal,nullptr });
			}
			break;
			case X::AST::ObType::UnaryOp:
				break;
			case X::AST::ObType::PipeOp:
				break;
			case X::AST::ObType::In:
			{
				auto& left = valueStack.top();
				valueStack.pop();
				auto& right = valueStack.top();
				valueStack.pop();
				X::Value retVal;
				auto ok = static_cast<X::AST::InOp*>(pExp)->ExpRun(rt, left, right, retVal);
				valueStack.push({ pExp,ok,nullptr });
			}
			break;
			case X::AST::ObType::Var:
			{
				AST::ExecAction action;
				X::Value retVal;
				LValue lv;
				static_cast<AST::Var*>(pExp)->Exec(rt, action, nullptr, retVal, &lv);
				valueStack.push({ pExp,retVal,lv });
			}
			break;
			case X::AST::ObType::Str:
				break;
			case X::AST::ObType::Const:
				break;
			case X::AST::ObType::Number:
				break;
			case X::AST::ObType::Double:
				break;
			case X::AST::ObType::Param:
				break;
			case X::AST::ObType::List:
			{
				auto& expList = static_cast<X::AST::List*>(pExp)->GetList();
				auto size = expList.size();
				X::Data::List* pOutList = new X::Data::List();
				for (size_t i = 0; i < size; i++)
				{
					auto& item = valueStack.top();
					pOutList->Add(rt, item.v);
					valueStack.pop();
				}
				valueStack.push({ pExp,pOutList,nullptr });
			}
			break;
			case X::AST::ObType::Pair:
			{
				auto* pPair = static_cast<X::AST::PairOp*>(pExp);
				ExpValue left = { nullptr };
				if (pPair->GetL())
				{
					left = valueStack.top();
					valueStack.pop();
				}
				ExpValue right = { nullptr };
				if (pPair->GetR())
				{
					right = valueStack.top();
					valueStack.pop();
				}
				X::Value retVal;
				auto ok = pPair->ExpRun(rt, left, right, retVal);
				valueStack.push({ pExp,retVal,nullptr });
			}
			break;
			case X::AST::ObType::Dot:
			{
				auto& left = valueStack.top();
				valueStack.pop();
				X::Value retVal;
				LValue lv;
				auto ok = static_cast<X::AST::DotOp*>(pExp)->ExpRun(rt, pContext, left, retVal, &lv);
				valueStack.push({ pExp,retVal,lv });
			}
			break;
			case X::AST::ObType::Decor:
				break;
			case X::AST::ObType::Func:
				break;
			case X::AST::ObType::JitBlock:
				break;
			case X::AST::ObType::BuiltinFunc:
				break;
			case X::AST::ObType::Module:
				break;
			case X::AST::ObType::Block:
				break;
			case X::AST::ObType::Class:
				break;
			case X::AST::ObType::ReturnType:
				break;
			case X::AST::ObType::From:
			{
				X::AST::ExecAction exeAction;
				ExpValue expValue = { pExp };
				auto ok = static_cast<X::AST::From*>(pExp)->Exec(rt, exeAction, nullptr, expValue.v, &expValue.lv);
				valueStack.push(expValue);
			}
			break;
			case X::AST::ObType::ColonOp:
				break;
			case X::AST::ObType::CommaOp:
				break;
			case X::AST::ObType::SemicolonOp:
				break;
			case X::AST::ObType::FeedOp:
				break;
			case X::AST::ObType::ActionOp:
				break;
			case X::AST::ObType::As:
				break;
			case X::AST::ObType::For:
			{
				auto& rightValue = valueStack.top();
				valueStack.pop();
				if (rightValue.v.IsTrue())
				{
					//set pushed flag to false, so that this expression will be processed again
					expStack.top().second = false;
					//then expanding
					static_cast<X::AST::For*>(pExp)->Expanding(expStack);
					needPopExpStack = false;
				}
				else
				{
					needPopExpStack = true;
				}
			}
			break;
			case X::AST::ObType::While:
				break;
			case X::AST::ObType::If:
				break;
			case X::AST::ObType::ExternDecl:
				break;
			case X::AST::ObType::AwaitOp:
				break;
			case X::AST::ObType::Thru:
			{
				X::AST::ExecAction exeAction;
				ExpValue expValue = { pExp };
				auto ok = static_cast<X::AST::From*>(pExp)->Exec(rt, exeAction, nullptr, expValue.v, &expValue.lv);
				valueStack.push(expValue);
			}
			break;
			case X::AST::ObType::Deferred:
				break;
			case X::AST::ObType::Import:
			{
				X::Value retVal;
				static_cast<X::AST::Import*>(pExp)->ExpRun(rt, pContext, valueStack, retVal);
			}
			break;
			case X::AST::ObType::NamespaceVar:
				break;
			default:
				break;
			}
			if (needPopExpStack)
			{
				expStack.pop();
			}
			return true;
		}

	}
}