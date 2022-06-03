#pragma once

namespace XPython {
#define nil 0
struct String
{
	char* s;
	int size;
};
enum class OP_ID
{
	Parenthesis_L,
	Brackets_L,
	Curlybracket_L,
	Slash,
	Colon,
	Tab,
	Count
};
enum class Alias1
{
	None,
	Colon,
	Comma,
	OP_LR,//operator with Left and right parameter
	OP_R,//right parameter only
	OP_N,//No parameter
	Parenthesis_L,
	Brackets_L,
	Curlybracket_L,
	Tab,
	CR,//'\r'
	Slash
};

class Parser;
namespace AST {
	class Operator;
	class UnaryOp;
	class BinaryOp;
	class Value;
}
struct OpAction;
typedef AST::Operator* (*OpProc)(
	Parser* p, short opIndex);
typedef bool (*UnaryOpProc)(
	AST::UnaryOp* op,AST::Value& R, AST::Value& out);
typedef bool (*BinaryOpProc)(
	AST::BinaryOp* op,AST::Value& L, AST::Value& R, AST::Value& out);


#define Precedence_Reqular 1000
#define Precedence_Min 0

struct OpAction
{
	OpProc process = nil;
	UnaryOpProc unaryop = nil;
	BinaryOpProc binaryop = nil;
	int precedence = Precedence_Reqular;
};

}