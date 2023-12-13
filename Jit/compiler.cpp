#include "compiler.h"
#include <regex>

namespace X
{
	namespace Jit
	{
		bool JitCompiler::TranslateCode(
			std::string& funcName,
			std::string& code,
			FuncParseInfo& info)
		{
			bool bRet = false;
			//(?:) is non-capture group
			static std::regex rgx("\\t*def[\\t\\s]+([^\\(\\)]+)\\(([^\\\\(\\\\)]*)\\)[\\s\\t]*(?:->[\\s\\t]*([^:]+))?:");
			static std::regex rgx_var("([^:,]+)(?::([^,]+))?,?");
			//to parse def func_name([var:type]*)->var:
			std::smatch matches;
			if (std::regex_search(code, matches, rgx))
			{
				if (matches.size() >= 3)
				{
					std::string funcName = matches[1].str();
					std::string vars = matches[2].str();
					std::smatch var_matches;
					while (std::regex_search(vars, var_matches, rgx_var))
					{
						auto varCnt = var_matches.size();
						if (varCnt == 3)
						{
							info.parameters.push_back(std::make_pair(
								var_matches[1].str(), var_matches[2].str()
							));
						}
						vars = var_matches.suffix();
					}
					if (matches.size() == 4)
					{
						info.retType = matches[3].str();
					}
					std::string body = matches.suffix();
					std::regex commentTag("\"\"\"");
					info.body = std::regex_replace(body, commentTag, "");
					bRet = true;
				}
			}
			return bRet;
		}
	}
}