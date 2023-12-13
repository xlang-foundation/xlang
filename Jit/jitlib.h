#pragma once
#include <string>
#include "jitblock.h"
#include "list.h"
#include "exp.h"
#include "op.h"
#include "port.h"

//one XLang moudle( one file) use one CodeGen to generate code
//so inside this file, will include Func and/or class code

namespace X 
{
	namespace Jit
	{
		class JitLib
		{
			std::string m_path;
			std::string m_moduleName;//module name without ext
		public:
			FORCE_INLINE std::string& Path()
			{
				return m_path;
			}
			FORCE_INLINE std::string& ModuleName()
			{
				return m_moduleName;
			}
			JitLib(std::string& moduleName)
			{
				std::string right;
				auto pos = moduleName.rfind(Path_Sep);
				if (pos != moduleName.npos)
				{
					m_path = moduleName.substr(0, pos);
					right = moduleName.substr(pos + 1);
				}
				else
				{
					right = moduleName;
				}
				pos = right.rfind('.');
				if (pos != right.npos)
				{
					m_moduleName = right.substr(0, pos);
				}
				else
				{
					m_moduleName = right;
				}
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
			bool BuildCode(int moduleIndex, std::string strJitFolder,
				std::vector<std::string>& srcs, std::vector<std::string>& exports);
			bool Build()
			{
				return true;
			}
		};
	}
}