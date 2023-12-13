#pragma once
#include <string>
#include "jitblock.h"
#include "list.h"
#include "exp.h"
#include "op.h"

//one XLang moudle( one file) use one CodeGen to generate code
//so inside this file, will include Func and/or class code

namespace X 
{
	class JitLib
	{
	public:
		JitLib()
		{

		}
		~JitLib()
		{

		}
		//acording define in this block with Input parameter and return value type
		//to geneate function head
		FORCE_INLINE std::string DeclareFuncHead(AST::JitBlock* pBlock)
		{
			std::string funcHead;
			std::string funcName = pBlock->GetName();
			auto* params = pBlock->GetParams();
			auto& paramList = params->GetList();
			auto& strRetType = pBlock->GetRetType();
			for (auto* i : paramList)
			{
				std::string strVarName;
				std::string strVarType;
				Value defaultValue;
				switch (i->m_type)
				{
				case AST::ObType::Var:
				{
					auto* varName = dynamic_cast<AST::Var*>(i);
					String& szName = varName->GetName();
					strVarName = std::string(szName.s, szName.size);
				}
				break;
				case AST::ObType::Assign:
				{
					auto* assign = dynamic_cast<AST::Assign*>(i);
					auto* varName = dynamic_cast<AST::Var*>(assign->GetL());
					String& szName = varName->GetName();
					strVarName = std::string(szName.s, szName.size);
					AST::Expression* defVal = assign->GetR();
					auto* pExprForDefVal = new Data::Expr(defVal);
					defaultValue = Value(pExprForDefVal);
				}
				break;
				case AST::ObType::Param:
				{
					auto* param = dynamic_cast<AST::Param*>(i);
					param->Parse(strVarName, strVarType, defaultValue);
				}
				break;
				}

			}
			return funcHead;
		}
		FORCE_INLINE std::string DeclareClassHead(AST::JitBlock* pBlock)
		{
			std::string classHead;
			return classHead;
		}
		bool AddBlock(AST::JitBlock* pBlock)
		{
			return true;
		}
		bool Build()
		{
			return true;
		}
	};
}