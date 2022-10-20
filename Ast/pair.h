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
		bool ParentRun(XlangRuntime* rt, XObj* pContext, Value& v, LValue* lValue);
		bool BracketRun(XlangRuntime* rt, XObj* pContext, Value& v, LValue* lValue);
		bool CurlyBracketRun(XlangRuntime* rt, XObj* pContext, Value& v, LValue* lValue);
		bool TableBracketRun(XlangRuntime* rt, XObj* pContext, Value& v, LValue* lValue);
		bool GetItemFromList(XlangRuntime* rt, XObj* pContext,
			Data::List* pDataList, Expression* r,
			Value& v, LValue* lValue);
		bool GetItemFromDict(XlangRuntime* rt, XObj* pContext,
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
		virtual bool Run(XlangRuntime* rt, XObj* pContext, Value& v, LValue* lValue = nullptr) override;
	};
}
}