#pragma once
#include <string>
#include <vector>
#include <unordered_map>

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
};

}