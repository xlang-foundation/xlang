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
	Add,
	Minus,
	Multiply,
};

}