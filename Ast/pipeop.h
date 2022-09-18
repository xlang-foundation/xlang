#pragma once
#include "exp.h"
#include "op.h"
#include "object.h"
#include "dotop.h"
#include "pair.h"
#include "function.h"

namespace X
{
	namespace AST
	{
		class PipeOp :
			virtual public DotOp
		{
		public:
			PipeOp():
				Operator(), BinaryOp(), DotOp()
			{
				m_type = ObType::PipeOp;
			}
			PipeOp(short idx):
				Operator(idx),
				BinaryOp(idx),
				DotOp(idx,1)
			{
				m_type = ObType::PipeOp;
			}
			virtual void ScopeLayout() override
			{
				if (L) L->ScopeLayout();
				if (R) R->ScopeLayout();
			}
			virtual void SetL(Expression* l) override
			{
				BinaryOp::SetL(l);
				if (L)
				{
					if (L->m_type == ObType::Var)
					{
						L->SetIsLeftValue(true);
					}
				}
			}
			virtual void SetR(Expression* r) override
			{
				BinaryOp::SetR(r);
				if (r)
				{
					r->SetIsLeftValue(true);
				}
			}
			bool SimpleProcess(Runtime* rt, XObj* pContext, Value& v_l,
				Expression* expR,Value& v, LValue* lValue)
			{
				bool bRet = true;
				Value v_r;
				LValue lValue_R = nullptr;
				if (!expR->Run(rt, pContext, v_r, &lValue_R))
				{
					return false;
				}
				if (lValue_R)
				{
					*lValue_R = v_l;
					//todo: maybe not correct
					v = v_l;//set back for next pipe
				}
				else
				{
					if (v_r.IsObject())
					{
						auto* pObj = dynamic_cast<Data::Object*>(v_r.GetObj());
						if (pObj->GetType() == ObjType::Function)
						{
							auto* pFuncObj = dynamic_cast<Data::Function*>(pObj);
							ARGS params;
							KWARGS kwparams;
							params.push_back(v_l);
							bRet = pFuncObj->Call(rt, pContext,params, kwparams, v);
						}
					}
					else
					{
						expR->Set(rt, pContext, v_l);
					}
					//todo: maybe not correct
					v = v_r;//set back for next pipe
				}
				return bRet;

			}
			bool Process(Runtime* rt, XObj* pContext, Value& val_l, Expression* expR, Value& v, LValue* lValue)
			{
				bool bOK = true;
				//do one pipe with x|expR
				if (val_l.IsObject() && expR->m_type == ObType::Pair)
				{
					PairOp* pPair0 = dynamic_cast<PairOp*>(expR);
					auto* pairL = pPair0->GetL();
					if (pairL == nullptr)
					{
						auto* pairR = pPair0->GetR();
						bOK = pairR->Run(rt, pContext, v, lValue);
						return bOK;
					}
					Value callList;
					LValue lValue0 = nullptr;
					bool bOK = DotProcess(rt, pContext, val_l, pairL, callList, &lValue0);

					ARGS params;
					KWARGS kwParams;
					if (R)
					{
						bOK = GetParamList(rt, pPair0->GetR(), params, kwParams);
						if (!bOK)
						{
							return bOK;
						}
					}
					Data::Object* obj = dynamic_cast<Data::Object*>(callList.GetObj());
					if (obj)
					{
						bOK = obj->Call(rt, pContext,params, kwParams, v);
					}
				}
				else
				{
					bOK = SimpleProcess(rt, pContext, val_l, expR, v, lValue);
				}
				return bOK;
			}
			bool RunOne(Runtime* rt, XObj* pContext,
				Value& v_l, Expression* R1,
				Value& v, LValue* lValue)
			{
				bool bOK = true;
				if (R1->m_type == ObType::PipeOp)
				{
					PipeOp* pPipeOp = dynamic_cast<PipeOp*>(R1);
					Expression* L2 = pPipeOp->GetL();
					Expression* R2 = pPipeOp->GetR();
					Value v_l2;
					LValue v_lval2 = nullptr;
					bOK = Process(rt, pContext, v_l, L2, v_l2, &v_lval2);
					if (bOK)
					{
						L2->Set(rt, pContext, v_l);
						bOK = RunOne(rt, pContext, v_l2, R2, v, lValue);
					}
				}
				else
				{
					bOK = Process(rt, pContext, v_l, R1, v, lValue);
					if (bOK)
					{
						R1->Set(rt, pContext, v_l);
					}
				}
				return bOK;
			}
			virtual bool Run(Runtime* rt, XObj* pContext, Value& v, LValue* lValue = nullptr) override
			{
				if (!L || !R)
				{
					return false;
				}
				Value v_l;
				if (!L->Run(rt, pContext, v_l))
				{
					return false;
				}
				return RunOne(rt, pContext, v_l, R, v, lValue);
			}
		};
	}
}