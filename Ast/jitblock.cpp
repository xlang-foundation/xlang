#include "jitblock.h"
#include "object.h"
#include "function.h"

namespace X
{
	namespace AST
	{
		void JitBlock::ScopeLayout()
		{
			Scope* pMyScope = GetScope();
			if (pMyScope)
			{
				std::string strName(m_Name.s, m_Name.size);
				m_Index = pMyScope->AddOrGet(strName, false);
			}
			//process parameters' default values
			if (Params)
			{
				auto& list = Params->GetList();
				//this case happened in lambda function
				for (auto i : list)
				{
					std::string strVarName;
					std::string strVarType;
					Value defaultValue;
					switch (i->m_type)
					{
					case ObType::Var:
					{
						Var* varName = dynamic_cast<Var*>(i);
						String& szName = varName->GetName();
						strVarName = std::string(szName.s, szName.size);
					}
					break;
					case ObType::Assign:
					{
						Assign* assign = dynamic_cast<Assign*>(i);
						Var* varName = dynamic_cast<Var*>(assign->GetL());
						String& szName = varName->GetName();
						strVarName = std::string(szName.s, szName.size);
						Expression* defVal = assign->GetR();
						auto* pExprForDefVal = new Data::Expr(defVal);
						defaultValue = Value(pExprForDefVal);
					}
					break;
					case ObType::Param:
					{
						Param* param = dynamic_cast<Param*>(i);
						param->Parse(strVarName, strVarType, defaultValue);
					}
					break;
					}
					int idx = m_pMyScope->AddOrGet(strVarName, false);
					m_IndexofParamList.push_back(idx);
				}
				Params->ScopeLayout();
			}
		}
		bool JitBlock::Exec(XlangRuntime* rt, ExecAction& action, XObj* pContext,
			Value& v, LValue* lValue)
		{
			//when build from stream, n_Index stored into Stream,but m_scope is not
			if (m_Index == -1 || m_scope == nullptr)
			{
				ScopeLayout();
				//for lambda function, no name, so skip this check
				if (m_Name.size > 0 && m_Index == -1)
				{
					return false;
				}
			}
			//AST::JitFunc* jitFunc = new AST::JitFunc(this);
			Data::Function* f = new Data::Function(this);
			Value v0(f);
			if (m_Index >= 0)
			{
				m_scope->Set(rt, pContext, m_Index, v0);
			}
			v = v0;
			return true;
		}

	}
}