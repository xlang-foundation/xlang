#pragma once
#include "exp.h"
#include "op.h"

namespace X
{
	namespace Data { class List; class Dict; }
namespace AST
{
	class PairOp :
		virtual public BinaryOp
	{
		short m_preceding_token = 0;
		bool ParentRun(Runtime* rt, XObj* pContext, Value& v, LValue* lValue);
		bool BracketRun(Runtime* rt, XObj* pContext, Value& v, LValue* lValue);
		bool CurlyBracketRun(Runtime* rt, XObj* pContext, Value& v, LValue* lValue);
		bool TableBracketRun(Runtime* rt, XObj* pContext, Value& v, LValue* lValue);
		bool GetItemFromList(Runtime* rt, XObj* pContext,
			Data::List* pDataList, Expression* r,
			Value& v, LValue* lValue);
		bool GetItemFromDict(Runtime* rt, XObj* pContext,
			Data::Dict* pDataDict, Expression* r,
			Value& v, LValue* lValue);
	public:
		PairOp() :
			Operator(),
			BinaryOp()
		{
			m_type = ObType::Pair;
		}
		PairOp(short opIndex, short preceding_token) :
			Operator(opIndex),
			BinaryOp(opIndex)
		{
			m_preceding_token = preceding_token;
			m_type = ObType::Pair;
		}
		short GetPrecedingToken()
		{
			return m_preceding_token;
		}
		virtual bool Run(Runtime* rt, XObj* pContext, Value& v, LValue* lValue = nullptr) override;
	};
}
}