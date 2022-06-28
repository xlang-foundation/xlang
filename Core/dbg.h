#pragma once
#include "exp.h"
#include "runtime.h"
#include "manager.h"
#include "module.h"
#include "scope.h"
#include <vector>
#include <iostream>

namespace X
{
class Dbg
{
	Runtime* m_rt = nullptr;
public:
	inline Dbg(Runtime* rt)
	{
		m_rt = rt;
	}
	void Loop(AST::Expression* exp, void* pContext)
	{
		while (true)
		{
			std::string input;
			std::getline(std::cin, input);
			if (input == "c" || input == "C")
			{
				m_rt->M()->SetDbg(AST::dbg::Continue);
				break;
			}
			else if (input == "s" || input == "S")
			{
				m_rt->M()->SetDbg(AST::dbg::Step);
				break;
			}
			else if(input == "l" || input == "L")
			{
				AST::Scope* pMyScope = exp->GetScope();
				auto l_names = pMyScope->GetVarNames();
				for (auto& l_name : l_names)
				{
					AST::Value vDbg;
					pMyScope->Get(m_rt, pContext, l_name, vDbg);
					std::cout << l_name << ":" << vDbg.ToString() << std::endl;
				}
			}
			else
			{
				AST::Scope* pMyScope = exp->GetScope();
				auto pos = input.find("show");
				if (pos != std::string::npos)
				{
					std::string strParas = input.substr(pos + 4);
					std::vector<std::string> params =
						split(strParas, ',');
					if (params.size() == 0)
					{
						params.push_back(strParas);
					}
					for (auto& param : params)
					{
						AST::Value vDbg;
						if (pMyScope)
						{
							pMyScope->Get(m_rt, pContext, param, vDbg);
						}
						std::cout <<vDbg.ToString() << std::endl;
					}
				}

			}
			std::cout << ">>";
		}
	}
	inline bool Check(AST::Value& lineRet,AST::Expression* exp, void* pContext)
	{
		if (m_rt->M()->GetDbg() == AST::dbg::Step)
		{
			std::cout << lineRet.ToString() << std::endl;
			int line = exp->GetStartLine();
			std::cout << "(" << line << ",(c)ontinue,(s)tep)>>";
			Loop(exp,pContext);
		}
		return true;
	}
};
}