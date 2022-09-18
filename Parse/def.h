#pragma once
#include <string>

namespace X 
{
#define nil 0
#define MAX_VAL(x,y) ((x)>(y)?(x):(y))
#define MIN_VAL(x,y) ((x)<(y)?(x):(y))
struct String
{
	char* s;
	int size;
};

enum class OP_ID
{
	None,
	Parenthesis_L,
	Brackets_L,
	Curlybracket_L,
	TableBracket_L,
	Slash,
	Colon,
	Comma,
	Tab,
	//	"=", "+=", "-=", "*=", "/=", "%=", "//=",
	Equ,AddEqu,MinusEqu,MulEqu,DivEqu,ModEqu,FloorDivEqu,
	//	"**=", "&=", "|=", "^=", ">>=", "<<="
	PowerEqu,AndEqu,OrEqu,NotEqu,RightShiftEqu,LeftShitEqu,
	Count
};

class Parser;
class Runtime;
class Value;
namespace AST 
{
	class Operator;
	class UnaryOp;
	class BinaryOp;
	typedef unsigned long long ExpId;
}
struct OpAction;
typedef AST::Operator* (*OpProc)(
	Parser* p, short opIndex);
typedef bool (*UnaryOpProc)(
	Runtime* rt, AST::UnaryOp* op,X::Value& R, X::Value& out);
typedef bool (*BinaryOpProc)(
	Runtime* rt, AST::BinaryOp* op,X::Value& L, X::Value& R, X::Value& out);

#define Precedence_High1 201
#define Precedence_High 200
#define Precedence_Reqular 100
#define Precedence_LOW2 80
#define Precedence_LOW1 60
#define Precedence_VERYLOW 20
#define Precedence_Min 0

struct OpAction
{
	OpProc process = nil;
	UnaryOpProc unaryop = nil;
	BinaryOpProc binaryop = nil;
	int precedence = Precedence_Reqular;
	OP_ID opId = OP_ID::None;
};
struct OpInfo
{
	int id;
	std::string name;
	OpAction act;
};
}