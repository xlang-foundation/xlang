#pragma once
#include "exp.h"
#include "op.h"

namespace X
{
	namespace Data { class List; class Dict; class Tensor; class mSet; }
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
		bool GetItemFromPackage(XlangRuntime* rt, XObj* pContext,
			Data::Object* pPackage, Expression* r,
			Value& v, LValue* lValue);
		bool GetItemFromTensor(XlangRuntime* rt, XObj* pContext,
			Data::Tensor* pTensor, Expression* r,
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
		inline virtual void SetIsLeftValue(bool b) override
		{
			if (R && R->m_type == AST::ObType::List)
			{
				auto* pList = dynamic_cast<AST::List*>(R);
				if (pList)
				{
					auto& list = pList->GetList();
					for (auto* l : list)
					{
						l->SetIsLeftValue(b);
					}
				}
			}
			else if(R)
			{
				R->SetIsLeftValue(b);
			}
		}
		virtual bool Set(XlangRuntime* rt, XObj* pContext, Value& v) override;
		inline virtual bool SetArry(XlangRuntime* rt, XObj* pContext, std::vector<Value>& ary) override
		{
			if (ary.size() == 0)
			{
				return false;
			}
			if (R && R->m_type == AST::ObType::List)
			{
				auto* pList = dynamic_cast<AST::List*>(R);
				if (pList)
				{
					auto& list = pList->GetList();
					int size_list = (int)list.size();
					int size_ary = (int)ary.size();
					for (int i=0;i< size_list;i++)
					{
						if (i < size_ary)
						{
							list[i]->Set(rt, pContext, ary[i]);
						}
					}
				}
			}
			else if (R && R->m_type == AST::ObType::Var)
			{
				R->Set(rt, pContext, ary[0]);
			}
			else
			{
				R->SetArry(rt, pContext, ary);
			}
			return true;
		}
		virtual bool Exec(XlangRuntime* rt,ExecAction& action, XObj* pContext, Value& v, LValue* lValue = nullptr) override;
	};
}
}