#pragma once

#include "object.h"
#include <string>
namespace X 
{
namespace Data 
{
	class Str
		:public Object
	{
	protected:
		std::string m_s;
	public:
		Str(String s)//from constant
		{//new copy
			m_t = Type::Str;
			m_s = std::string(s.s, s.size);
		}
		virtual bool Call(void* pLineExpr, std::vector<AST::Value>& params,
			std::unordered_map<std::string, AST::Value>& kwParams,
			AST::Value& retValue)
		{
			return true;
		}
	};
}
}