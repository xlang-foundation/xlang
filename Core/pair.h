#pragma once
#include "exp.h"
#include "op.h"

namespace X
{
	namespace Data { class List; class Dict; }
namespace AST
{
	class PairOp :
		public BinaryOp
	{
		short m_preceding_token = 0;
		bool ParentRun(Runtime* rt, void* pContext, Value& v, LValue* lValue);
		bool BracketRun(Runtime* rt, void* pContext, Value& v, LValue* lValue);
		bool CurlyBracketRun(Runtime* rt, void* pContext, Value& v, LValue* lValue);
		bool GetItemFromList(Runtime* rt, void* pContext,
			Data::List* pDataList, Expression* r,
			Value& v, LValue* lValue);
		bool GetItemFromDict(Runtime* rt, void* pContext,
			Data::Dict* pDataDict, Expression* r,
			Value& v, LValue* lValue);
	public:
		PairOp(short opIndex, short preceding_token) :
			BinaryOp(opIndex)
		{
			m_preceding_token = preceding_token;
			m_type = ObType::Pair;
		}
		short GetPrecedingToken()
		{
			return m_preceding_token;
		}
		virtual bool Run(Runtime* rt, void* pContext, Value& v, LValue* lValue = nullptr) override;
	};
}
}