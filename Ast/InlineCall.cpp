#include "block.h"
#include "var.h"
#include "func.h"
#include "builtin.h"
#include "module.h"
#include "utility.h"
#include "pipeop.h"
#include "pair.h"
#include "xclass.h"
#include "import.h"
#include "feedop.h"
#include "await.h"
#include "namespace_var.h"

namespace X
{
	namespace AST
	{
		extern FORCE_INLINE  bool ExpExec(Expression* pExp,
			XlangRuntime* rt,
			ExecAction& action,
			XObj* pContext,
			Value& v,
			LValue* lValue)
		{
			bool bOK = false;
			auto expType =  pExp->m_type;
			switch (expType)
			{
			case X::AST::ObType::InlineComment:
				bOK = static_cast<InlineComment*>(pExp)->Exec(rt, action, pContext, v, lValue);
				break;
			case X::AST::ObType::Assign:
				bOK = static_cast<Assign*>(pExp)->Exec(rt, action, pContext, v, lValue);
				break;
			case X::AST::ObType::BinaryOp:
				bOK = static_cast<BinaryOp*>(pExp)->Exec(rt, action, pContext, v, lValue);
				break;
			case X::AST::ObType::UnaryOp:
				bOK = static_cast<UnaryOp*>(pExp)->Exec(rt, action, pContext, v, lValue);
				break;
			case X::AST::ObType::PipeOp:
				bOK = static_cast<PipeOp*>(pExp)->Exec(rt, action, pContext, v, lValue);
				break;
			case X::AST::ObType::In:
				bOK = static_cast<InOp*>(pExp)->Exec(rt, action, pContext, v, lValue);
				break;
			case X::AST::ObType::Range:
				bOK = static_cast<Range*>(pExp)->Exec(rt, action, pContext, v, lValue);
				break;
			case X::AST::ObType::Var:
				bOK = static_cast<Var*>(pExp)->Exec(rt, action, pContext, v, lValue);
				break;
			case X::AST::ObType::Str:
				bOK = static_cast<Str*>(pExp)->Exec(rt, action, pContext, v, lValue);
				break;
			case X::AST::ObType::Const:
				bOK = static_cast<XConst*>(pExp)->Exec(rt, action, pContext, v, lValue);
				break;
			case X::AST::ObType::Number:
				bOK = static_cast<Number*>(pExp)->Exec(rt, action, pContext, v, lValue);
				break;
			case X::AST::ObType::Double:
				bOK = static_cast<Double*>(pExp)->Exec(rt, action, pContext, v, lValue);
				break;
			case X::AST::ObType::Param:
				bOK = static_cast<Param*>(pExp)->Exec(rt, action, pContext, v, lValue);
				break;
			case X::AST::ObType::List:
				bOK = static_cast<List*>(pExp)->Exec(rt, action, pContext, v, lValue);
				break;
			case X::AST::ObType::Pair:
				bOK = static_cast<PairOp*>(pExp)->Exec(rt, action, pContext, v, lValue);
				break;
			case X::AST::ObType::Dot:
				bOK = static_cast<DotOp*>(pExp)->Exec(rt, action, pContext, v, lValue);
				break;
			case X::AST::ObType::Decor:
				bOK = static_cast<Decorator*>(pExp)->Exec(rt, action, pContext, v, lValue);
				break;
			case X::AST::ObType::Func:
				bOK = static_cast<Func*>(pExp)->Exec(rt, action, pContext, v, lValue);
				break;
			case X::AST::ObType::BuiltinFunc:
				bOK = static_cast<ExternFunc*>(pExp)->Exec(rt, action, pContext, v, lValue);
				break;
			case X::AST::ObType::Module:
				bOK = static_cast<Module*>(pExp)->Exec(rt, action, pContext, v, lValue);
				break;
			case X::AST::ObType::Block:
				bOK = static_cast<Block*>(pExp)->Exec(rt, action, pContext, v, lValue);
				break;
			case X::AST::ObType::Class:
				bOK = static_cast<XClass*>(pExp)->Exec(rt, action, pContext, v, lValue);
				break;
			case X::AST::ObType::From:
				bOK = static_cast<From*>(pExp)->Exec(rt, action, pContext, v, lValue);
				break;
			case X::AST::ObType::ColonOp:
				bOK = static_cast<ColonOP*>(pExp)->Exec(rt, action, pContext, v, lValue);
				break;
			case X::AST::ObType::CommaOp:
				bOK = static_cast<CommaOp*>(pExp)->Exec(rt, action, pContext, v, lValue);
				break;
			case X::AST::ObType::SemicolonOp:
				bOK = static_cast<SemicolonOp*>(pExp)->Exec(rt, action, pContext, v, lValue);
				break;
			case X::AST::ObType::FeedOp:
				bOK = static_cast<FeedOp*>(pExp)->Exec(rt, action, pContext, v, lValue);
				break;
			case X::AST::ObType::ActionOp:
				bOK = static_cast<ActionOperator*>(pExp)->Exec(rt, action, pContext, v, lValue);
				break;
			case X::AST::ObType::As:
				bOK = static_cast<AsOp*>(pExp)->Exec(rt, action, pContext, v, lValue);
				break;
			case X::AST::ObType::For:
				bOK = static_cast<For*>(pExp)->Exec(rt, action, pContext, v, lValue);
				break;
			case X::AST::ObType::While:
				bOK = static_cast<While*>(pExp)->Exec(rt, action, pContext, v, lValue);
				break;
			case X::AST::ObType::If:
				bOK = static_cast<If*>(pExp)->Exec(rt, action, pContext, v, lValue);
				break;
			case X::AST::ObType::ExternDecl:
				bOK = static_cast<ExternDecl*>(pExp)->Exec(rt, action, pContext, v, lValue);
				break;
			case X::AST::ObType::AwaitOp:
				bOK = static_cast<AwaitOp*>(pExp)->Exec(rt, action, pContext, v, lValue);
				break;
			case X::AST::ObType::Thru:
				bOK = static_cast<ThruOp*>(pExp)->Exec(rt, action, pContext, v, lValue);
				break;
			case X::AST::ObType::Deferred:
				bOK = static_cast<DeferredOP*>(pExp)->Exec(rt, action, pContext, v, lValue);
				break;
			case X::AST::ObType::Import:
				bOK = static_cast<Import*>(pExp)->Exec(rt, action, pContext, v, lValue);
				break;
			case X::AST::ObType::NamespaceVar:
				bOK = static_cast<NamespaceVar*>(pExp)->Exec(rt, action, pContext, v, lValue);
				break;
			default:
				break;
			}
			return bOK;
		}
		extern FORCE_INLINE bool ExpSet(Expression* pExp,
			XlangRuntime* rt,
			XObj* pContext,
			Value& v)
		{
			bool bOK = false;
			auto expType = pExp->m_type;
			switch (expType)
			{
			case X::AST::ObType::Var:
				bOK = static_cast<Var*>(pExp)->Set(rt, pContext, v);
				break;
			default:
				break;
			}
			return bOK;
		}
	}
}