/*
Copyright (C) 2024 The XLang Foundation
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "number.h"
#include <cmath>

namespace X
{
	// Helper: get hex digit value, returns -1 if invalid
	static inline int HexDigitValue(char c)
	{
		if (c >= '0' && c <= '9') return c - '0';
		if (c >= 'a' && c <= 'f') return c - 'a' + 10;
		if (c >= 'A' && c <= 'F') return c - 'A' + 10;
		return -1;
	}

	// Helper: check if char is hex digit
	static inline bool IsHexDigit(char c)
	{
		return HexDigitValue(c) >= 0;
	}

	// Parse hex float: format is 0x[hex].[hex]p[+-][decimal]
	// Example: 0x1.Fp-3 = 1.9375 * 2^-3 = 0.2421875
	static ParseState ParseHexFloat(char* start, char* end, double& dVal)
	{
		char* p = start;

		// Parse integer part (hex digits before .)
		double intPart = 0;
		bool hasIntPart = false;
		while (p < end && (IsHexDigit(*p) || *p == '_'))
		{
			if (*p != '_')
			{
				intPart = intPart * 16 + HexDigitValue(*p);
				hasIntPart = true;
			}
			p++;
		}

		// Parse fractional part (hex digits after .)
		double fracPart = 0;
		bool hasFracPart = false;
		if (p < end && *p == '.')
		{
			p++; // skip '.'
			double fracMultiplier = 1.0 / 16.0;
			while (p < end && (IsHexDigit(*p) || *p == '_'))
			{
				if (*p != '_')
				{
					fracPart += HexDigitValue(*p) * fracMultiplier;
					fracMultiplier /= 16.0;
					hasFracPart = true;
				}
				p++;
			}
		}

		// Must have at least integer or fractional part
		if (!hasIntPart && !hasFracPart)
		{
			return ParseState::Wrong_Fmt;
		}

		// Must have 'p' or 'P' for exponent
		if (p >= end || (*p != 'p' && *p != 'P'))
		{
			return ParseState::Wrong_Fmt;
		}
		p++; // skip 'p' or 'P'

		// Parse exponent sign
		int expSign = 1;
		if (p < end)
		{
			if (*p == '+') p++;
			else if (*p == '-') { expSign = -1; p++; }
		}

		// Parse exponent value (decimal digits)
		int exponent = 0;
		bool hasExponent = false;
		while (p < end)
		{
			char c = *p;
			if (c >= '0' && c <= '9')
			{
				exponent = exponent * 10 + (c - '0');
				hasExponent = true;
			}
			else if (c == '_')
			{
				// allow underscore in exponent
			}
			else
			{
				return ParseState::Wrong_Fmt;
			}
			p++;
		}

		if (!hasExponent)
		{
			return ParseState::Wrong_Fmt;
		}

		exponent *= expSign;

		// Compute final value: (intPart + fracPart) * 2^exponent
		dVal = (intPart + fracPart) * std::pow(2.0, exponent);
		return ParseState::Double;
	}

	// Parse hex/bin/oct after the prefix has been identified
	// Called with str pointing AFTER "0x", "0b", or "0o"
	// prefix is 'x','X','b','B','o','O'
	static ParseState ParseHexBinOctNumberInternal(char prefix, String& str,
		double& dVal, long long& llVal)
	{
		if (str.s == nil || str.size == 0)
		{
			return ParseState::Wrong_Fmt;
		}

		char* p = str.s;
		char* end = str.s + str.size;
		long long value = 0;
		bool hasDigit = false;
		bool meetJ = false;

		// Check for 'j'/'J' suffix (complex number)
		if (str.size > 0 && (*(end - 1) == 'j' || *(end - 1) == 'J'))
		{
			meetJ = true;
			end--;
		}

		if (prefix == 'x' || prefix == 'X')
		{
			// Check if this is a hex float (contains . or p/P)
			bool isHexFloat = false;
			for (char* scan = p; scan < end; scan++)
			{
				if (*scan == '.' || *scan == 'p' || *scan == 'P')
				{
					isHexFloat = true;
					break;
				}
			}

			if (isHexFloat && !meetJ)
			{
				return ParseHexFloat(p, end, dVal);
			}

			// Parse as hex integer
			while (p < end)
			{
				char c = *p++;
				if (c == '_') continue;
				int digitVal = HexDigitValue(c);
				if (digitVal < 0)
				{
					return ParseState::Wrong_Fmt;
				}
				value = value * 16 + digitVal;
				hasDigit = true;
			}
		}
		else if (prefix == 'b' || prefix == 'B')
		{
			// Binary: 0, 1, underscore
			while (p < end)
			{
				char c = *p++;
				if (c == '_') continue;
				if (c != '0' && c != '1')
				{
					return ParseState::Wrong_Fmt;
				}
				value = value * 2 + (c - '0');
				hasDigit = true;
			}
		}
		else if (prefix == 'o' || prefix == 'O')
		{
			// Octal: 0-7, underscore
			while (p < end)
			{
				char c = *p++;
				if (c == '_') continue;
				if (c < '0' || c > '7')
				{
					return ParseState::Wrong_Fmt;
				}
				value = value * 8 + (c - '0');
				hasDigit = true;
			}
		}
		else
		{
			return ParseState::Wrong_Fmt;
		}

		if (!hasDigit)
		{
			return ParseState::Wrong_Fmt;
		}

		llVal = value;
		if (meetJ)
		{
			dVal = (double)value;
			return ParseState::Complex;
		}
		return ParseState::Long_Long;
	}

	ParseState ParseNumber(String& str,
		double& dVal, long long& llVal)
	{
		ParseState st = ParseState::Null;
		if (str.s == nil || str.size == 0)
		{
			return st;
		}
		// Check for hex/oct/bin prefix: 0x, 0X, 0o, 0O, 0b, 0B
		if (str.size >= 2 && *str.s == '0')
		{
			char c = *(str.s + 1);
			if (c == 'x' || c == 'X' || c == 'o' || c == 'O' || c == 'b' || c == 'B')
			{
				String str2 = { str.s + 2, str.size - 2 };
				return ParseHexBinOctNumberInternal(c, str2, dVal, llVal);
			}
		}
		// Original decimal parsing logic
		long long primary[2] = { 0,0 };
		int digit_cnt[2] = { 0,0 };
		char* end = str.s + str.size;
		int it = 0;
		bool meetDot = false;
		bool meetJ = false;
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
			else if (c == '_')
			{
				// Python-style underscore separator - skip it
				continue;
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
			else if ((c == 'j' || c == 'J') && (p == end))//last one is j
			{
				meetJ = true;
			}
			else if ((c == 'e' || c == 'E') && !meetJ)
			{
				// Scientific notation
				int expSign = 1;
				int exponent = 0;
				bool hasExpDigit = false;

				if (p < end)
				{
					if (*p == '+') p++;
					else if (*p == '-') { expSign = -1; p++; }
				}

				while (p < end)
				{
					char ec = *p++;
					if (ec >= '0' && ec <= '9')
					{
						exponent = exponent * 10 + (ec - '0');
						hasExpDigit = true;
					}
					else if (ec == '_')
					{
						continue;
					}
					else if ((ec == 'j' || ec == 'J') && p == end)
					{
						meetJ = true;
					}
					else
					{
						correctSyntax = false;
						break;
					}
				}

				if (!hasExpDigit)
				{
					correctSyntax = false;
					break;
				}

				// Calculate value with exponent
				double baseVal;
				if (meetDot)
				{
					baseVal = (double)primary[1];
					for (int i = 0; i < digit_cnt[1]; i++)
						baseVal /= 10;
					baseVal += primary[0];
				}
				else
				{
					baseVal = (double)primary[0];
				}

				dVal = baseVal * std::pow(10.0, expSign * exponent);
				return meetJ ? ParseState::Complex : ParseState::Double;
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
				//todo: faster way?
				for (int i = 0; i < digit_cnt[1]; i++)
				{
					dVal /= 10;
				}
				dVal += primary[0];
				st = meetJ ? ParseState::Complex : ParseState::Double;
			}
			else
			{
				llVal = primary[0];
				dVal = digit_cnt[0];//reuse for count of digits
				if (meetJ)
				{
					st = ParseState::Complex;
					dVal = (double)llVal;
				}
				else
				{
					st = ParseState::Long_Long;
				}
			}
		}
		return st;
	}
}