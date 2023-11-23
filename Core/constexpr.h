#pragma once
#include "object.h"
#include "def.h"
#include <vector>
namespace X
{
	class XlangRuntime;
	namespace AST
	{
		class Expression;
	}
	namespace Data
	{
		struct ExpOne
		{
			OP_ID withPrevious =OP_ID::None;//multiple logcial expression with and/or
			OP_ID op;
			long long IndexOfArray=-1;//for${0},0 means current
			X::Value val;
		};
		class ConstExpr :
			virtual public XConstExpr,
			virtual public Object
		{
			OP_ID m_lastCombineOp = OP_ID::None;//for and/or
			std::vector<ExpOne> m_exprs;
			bool m_hasAction = false;
		public:
			ConstExpr():
				Object(),XConstExpr(0)
			{
			}
			bool hasAction() { return m_hasAction; }
			void Set(XlangRuntime* rt,AST::Expression* exp);
			FORCE_INLINE void Run(Object* pObj, long long curPos, bool& retVal)
			{
				retVal = true;
				for (auto& one : m_exprs)
				{
					bool curRetVal = true;
					X::Value itemVal;
					pObj->Get(curPos + one.IndexOfArray, itemVal);
					switch (one.op)
					{
					case OP_ID::Equal:
						curRetVal = (itemVal == one.val);
						break;
					case OP_ID::NotEqual:
						curRetVal = (itemVal != one.val);
						break;
					case OP_ID::Great:
						curRetVal = (itemVal > one.val);
						break;
					case OP_ID::Less:
						curRetVal = (itemVal < one.val);
						break;
					case OP_ID::GreatAndEqual:
						curRetVal = (itemVal >= one.val);
						break;
					case OP_ID::LessAndEqual:
						curRetVal = (itemVal <= one.val);
						break;
					default:
						break;
					}
					if (one.withPrevious == OP_ID::And)
					{
						retVal = retVal && curRetVal;
					}
					else if (one.withPrevious == OP_ID::Or)
					{
						retVal = retVal || curRetVal;
					}
					else
					{
						retVal = curRetVal;
					}
				}
			}
		};
	}
}