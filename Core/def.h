#pragma once

namespace XPython {
#define nil 0
struct String
{
	char* s;
	int size;
};
enum class Alias
{
	None,
	Func,
	Colon,
	Dot,
	Comma,
	Parenthesis_L,
	Brackets_L,
	Curlybracket_L,
	Tab,
	CR,//'\r'
	Slash,
	Invert, //'~'
};

enum class KWIndex
{
	KW_S = 0,
	def = 11,
	KW_E = 34,

	Assign_Op_S = 35,
	Assign_Op_E = 47,

	Arithmetic_Op_S = 48,

	Add = 48, //unary + or other meaning
	Minus = 49,//unary - or other meaning
	Multiply = 50,//*
	Arithmetic_Op_E = 54,

	Comparison_Op_S = 55,
	Comparison_Op_E = 60,

	Bitwise_Op_S = 61,

	Invert = 64,//unary invert

	Bitwise_Op_E = 66,

	Other_Op_S = 67,
	Parenthesis_L = 67,
	Parenthesis_R = 68,
	Bracket_L = 69,
	Brackets_R = 70,
	Curlybracket_L = 71,
	Curlybracket_R = 72,
	Colon = 73,//:
	Dot = 74,//.
	Comma = 75,//,
	Other_Op_E = 75,

	Misc_Op_S = 76,
	Newline = 78,
	Slash = 79,
	Misc_Op_E = 79,
	MaxCount
};

}