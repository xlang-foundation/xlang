#pragma once
#include <string>
#include <vector>
#include <unordered_map>

#if (WIN32)
#define SPRINTF sprintf_s
#else
#define SPRINTF snprintf
#endif

namespace X {
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
	Runtime* rt, AST::UnaryOp* op,AST::Value& R, AST::Value& out);
typedef bool (*BinaryOpProc)(
	Runtime* rt, AST::BinaryOp* op,AST::Value& L, AST::Value& R, AST::Value& out);
typedef std::vector<X::AST::Value> ARGS;
typedef std::unordered_map<std::string, X::AST::Value> KWARGS;

#define Precedence_High1 201
#define Precedence_High 200
#define Precedence_Reqular 100
#define Precedence_LOW2 80
#define Precedence_LOW1 60
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

typedef AST::Value (*EnumProc)(AST::Value& elm,unsigned long long idx);
}