#pragma once
#include "def.h"

namespace X 
{
	enum class ParseState
	{
		Wrong_Fmt,
		Null,
		Non_Number,
		Double,
		Long_Long
	};
	ParseState ParseHexBinOctNumber(String& str);
	ParseState ParseNumber(String& str,
		double& dVal, long long& llVal);
}