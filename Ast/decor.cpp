#include "decor.h"
#include "pair.h"
#include "object.h"

namespace X
{
	namespace AST
	{
		bool Decorator::Run(Runtime* rt, XObj* pContext, Value& v, LValue* lValue)
		{
			if (pContext == nullptr)
			{
				return true;//not called from decorated objct
			}
			bool bOK = true;
			if (R->m_type == ObType::Pair)
			{
				PairOp* pPairOp = dynamic_cast<PairOp*>(R);
				auto* pairL = pPairOp->GetL();
				if (pairL)
				{//Call Func
					Value lVal;
					bOK = pairL->Run(rt, pContext, lVal, lValue);
					if (!bOK || !lVal.IsObject())
					{
						return bOK;
					}
					ARGS params;
					KWARGS kwParams;
					auto* pairR = pPairOp->GetR();
					if (pairR)
					{
						bOK = GetParamList(rt, pairR, params, kwParams);
						if (!bOK)
						{
							return bOK;
						}
					}
					//kwParams.emplace(std::make_pair("origin", pContext));
					Data::Object* obj = dynamic_cast<Data::Object*>(lVal.GetObj());
					if (obj)
					{
						X::Value valTrailer(pContext);
						bOK = obj->CallEx(rt, pContext, params, kwParams, valTrailer,v);
					}
				}
			}
			return bOK;
		}
	}
}