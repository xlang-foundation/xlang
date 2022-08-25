#pragma once
#include <vector>
#include <string>
#include "def.h"
#include "parser.h"

namespace X 
{
	class OpRegistry;
	class RegOP
	{
		std::vector<std::string> ops;
	public:
		static std::vector<OpInfo> OPList;

		template<typename... Names>
		RegOP(Names... names)
		{
			ops = { names... };
			for (auto& n : ops)
			{
				AddOrGet(n);
			}
		}
		OpInfo& AddOrGet(std::string& name)
		{
			for (auto& i : OPList)
			{
				if (i.name == name)
				{
					return i;
				}
			}
			//not found
			OPList.push_back(OpInfo{ (int)OPList.size(),name });
			return OPList.back();
		}
		RegOP& SetId(OpRegistry* reg,OP_ID id);
		RegOP& SetIds(OpRegistry* reg, std::vector<OP_ID> ids);
		RegOP& SetPrecedence(int p)
		{
			for (auto& n : ops)
			{
				AddOrGet(n).act.precedence = p;
			}
			return *this;
		}
		RegOP& SetProcess(OpProc p)
		{
			for (auto& n : ops)
			{
				AddOrGet(n).act.process = p;
			}
			return *this;
		}
		RegOP& SetUnaryop(UnaryOpProc p)
		{
			for (auto& n : ops)
			{
				AddOrGet(n).act.unaryop = p;
			}
			return *this;
		}
		RegOP& SetBinaryop(BinaryOpProc p)
		{
			for (auto& n : ops)
			{
				AddOrGet(n).act.binaryop = p;
			}
			return *this;
		}
	};
	void BuildOps();
}
