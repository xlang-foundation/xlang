#include "number.h"
namespace X
{
	ParseState ParseHexBinOctNumber(String& str)
	{
		return ParseState::Null;
	}
	ParseState ParseNumber(String& str,
		double& dVal, long long& llVal)
	{
		ParseState st = ParseState::Null;
		if (str.s == nil || str.size == 0)
		{
			return st;
		}
		if (str.size >= 2 && *str.s == '0')
		{
			char c = *(str.s + 1);
			//for hex(0x),oct(0o),bin(0b), also require first letter is 0
			//so this is true if it is number
			if (c == 'x' || c == 'o' || c == 'b')
			{
				String str2 = { str.s + 2,str.size - 2 };
				return ParseHexBinOctNumber(str2);
			}
		}
		long long primary[2] = { 0,0 };
		int digit_cnt[2] = { 0,0 };
		char* end = str.s + str.size;
		int it = 0;
		bool meetDot = false;
		bool correctSyntax = true;
		char* p = str.s;
		while (p < end)
		{
			char c = *p++;
			if (c >= '0' && c <= '9')
			{
				primary[it] = primary[it] * 10 + c - '0';
				digit_cnt[it]++;
			}
			else if (c == '.')
			{
				if (meetDot)
				{//more than one
					//error
					correctSyntax = false;
					break;
				}
				meetDot = true;
				it++;
			}
			else
			{
				//error
				correctSyntax = false;
				break;
			}
		}
		if (correctSyntax)
		{
			if (meetDot)
			{
				dVal = (double)primary[1];
				for (int i = 0; i < digit_cnt[1]; i++)
				{
					dVal /= 10;
				}
				dVal += primary[0];
				st = ParseState::Double;
			}
			else
			{
				llVal = primary[0];
				dVal = digit_cnt[0];//reuse for count of digits
				st = ParseState::Long_Long;
			}
		}
		return st;
	}
}