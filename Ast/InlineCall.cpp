#include "block.h"
#include "var.h"
#include "func.h"
#include "builtin.h"
#include "module.h"
#include "utility.h"

namespace X
{
	namespace AST
	{
		extern inline bool ExpExec(Expression* pExp,
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
			case X::AST::ObType::Base:
				break;
			case X::AST::ObType::InlineComment:
				break;
			case X::AST::ObType::Assign:
				break;
			case X::AST::ObType::BinaryOp:
				break;
			case X::AST::ObType::UnaryOp:
				break;
			case X::AST::ObType::PipeOp:
				break;
			case X::AST::ObType::In:
				bOK = static_cast<InOp*>(pExp)->Exec(rt, action, pContext, v, lValue);
				break;
			case X::AST::ObType::Range:
				bOK = static_cast<Range*>(pExp)->Exec(rt, action, pContext, v, lValue);
				break;
			case X::AST::ObType::Var:
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
				break;
			case X::AST::ObType::Pair:
				break;
			case X::AST::ObType::Dot:
				break;
			case X::AST::ObType::Decor:
				break;
			case X::AST::ObType::Func:
				break;
			case X::AST::ObType::BuiltinFunc:
				break;
			case X::AST::ObType::Module:
				break;
			case X::AST::ObType::Block:
				break;
			case X::AST::ObType::Class:
				break;
			case X::AST::ObType::From:
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
				break;
			case X::AST::ObType::Deferred:
				break;
			case X::AST::ObType::Import:
				break;
			case X::AST::ObType::NamespaceVar:
				break;
			default:
				break;
			}
			return bOK;
		}
		extern inline bool ExpSet(Expression* pExp,
			XlangRuntime* rt,
			XObj* pContext,
			Value& v)
		{
			bool bOK = false;
			auto expType = pExp->m_type;
			switch (expType)
			{
			case X::AST::ObType::Base:
				break;
			case X::AST::ObType::InlineComment:
				break;
			case X::AST::ObType::Assign:
				break;
			case X::AST::ObType::BinaryOp:
				break;
			case X::AST::ObType::UnaryOp:
				break;
			case X::AST::ObType::PipeOp:
				break;
			case X::AST::ObType::In:
				break;
			case X::AST::ObType::Range:
				break;
			case X::AST::ObType::Var:
				bOK = static_cast<Var*>(pExp)->Set(rt, pContext, v);
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
				break;
			case X::AST::ObType::Pair:
				break;
			case X::AST::ObType::Dot:
				break;
			case X::AST::ObType::Decor:
				break;
			case X::AST::ObType::Func:
				break;
			case X::AST::ObType::BuiltinFunc:
				break;
			case X::AST::ObType::Module:
				break;
			case X::AST::ObType::Block:
				break;
			case X::AST::ObType::Class:
				break;
			case X::AST::ObType::From:
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
				break;
			case X::AST::ObType::Deferred:
				break;
			case X::AST::ObType::Import:
				break;
			case X::AST::ObType::NamespaceVar:
				break;
			default:
				break;
			}
			return bOK;
		}
	}
}