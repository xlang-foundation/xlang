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
		enum class ExpValueType
		{
			_Long,_Double,_Str
		};
		struct ExpOne
		{
			OP_ID withPrevious =OP_ID::None;//multiple logcial expression with and/or
			OP_ID op;
			long long IndexOfArray=-1;//for${0},0 means current
			//value to do with op for this IndexofArray
			ExpValueType valType;
			long _dummy = 0; //need to aligned to 8 bytes
			long long llval=0;
			double dVal=0.0;
			std::string strVal;
		};
		class ConstExpr :
			virtual public XConstExpr,
			virtual public Object
		{
			OP_ID m_lastCombineOp = OP_ID::None;//for and/or
			std::vector<ExpOne> m_exprs;
		public:
			ConstExpr():
				Object(),XConstExpr(0)
			{
			}
			void Set(XlangRuntime* rt,AST::Expression* exp);
		};
	}
}