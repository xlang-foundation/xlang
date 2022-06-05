#include "action.h"
#include "parser.h"
#include "exp.h"

namespace X {

void RegisterOps()
{
	RegOP("+")
	.SetUnaryop([](AST::UnaryOp* op,AST::Value& R, AST::Value& v) {
		v = R;//just keep as + does
		return true;
	})
	.SetBinaryop([](AST::BinaryOp* op,AST::Value& L, AST::Value& R, AST::Value& v) {
		v = L;
		v += R;
		return true;
	});
	RegOP("-")
	.SetUnaryop([](AST::UnaryOp* op,AST::Value& R, AST::Value& v) {
		v = AST::Value((long long)0);//set to 0
		v -= R;
		return true;
	})
	.SetBinaryop([](AST::BinaryOp* op,AST::Value& L, AST::Value& R, AST::Value& v) {
		v = L;
		v -= R;
		return true;
	});
	RegOP("*")
	.SetBinaryop([](AST::BinaryOp* op, AST::Value& L, AST::Value& R, AST::Value& v) {
			v = L;
			v *= R;
			return true;
	});
	RegOP(".")
	.SetBinaryop([](AST::BinaryOp* op, AST::Value& L, AST::Value& R, AST::Value& v) {
		int cnt = R.GetF();
		double d = (double)R.GetLongLong();
		for (int i = 0; i < cnt; i++)
		{
			d /= 10;
		}
		d += (double)L.GetLongLong();
		v = AST::Value(d);
		return true;
	});
	RegOP("/")
	.SetBinaryop([](AST::BinaryOp* op, AST::Value& L, AST::Value& R, AST::Value& v) {
		if (!R.IsZero())
		{
			v = L;
			v /= R;
			return true;
		}
		else
		{
			return false;
		}
	});
	RegOP("==")
	.SetBinaryop([](AST::BinaryOp* op, AST::Value& L, AST::Value& R, AST::Value& v) {
		v = AST::Value(L == R);
		return true;
	});
	RegOP("!=")
	.SetBinaryop([](AST::BinaryOp* op, AST::Value& L, AST::Value& R, AST::Value& v) {
		v = AST::Value(L != R);
		return true;
	});
	RegOP(">")
	.SetBinaryop([](AST::BinaryOp* op, AST::Value& L, AST::Value& R, AST::Value& v) {
		v = AST::Value(L > R);
		return true;
	});
	RegOP("<")
	.SetBinaryop([](AST::BinaryOp* op, AST::Value& L, AST::Value& R, AST::Value& v) {
		v = AST::Value(L < R);
		return true;
	});
	RegOP(">=")
	.SetBinaryop([](AST::BinaryOp* op, AST::Value& L, AST::Value& R, AST::Value& v) {
		v = AST::Value(L >= R);
		return true;
	});
	RegOP("<=")
	.SetBinaryop([](AST::BinaryOp* op, AST::Value& L, AST::Value& R, AST::Value& v) {
		v = AST::Value(L <= R);
		return true;
	});
	RegOP("and")
	.SetBinaryop([](AST::BinaryOp* op, AST::Value& L, AST::Value& R, AST::Value& v) {
		v = AST::Value(L.IsTrue() && R.IsTrue());
		return true;
	});
	RegOP("or")
	.SetBinaryop([](AST::BinaryOp* op, AST::Value& L, AST::Value& R, AST::Value& v) {
		v = AST::Value(L.IsTrue() || R.IsTrue());
		return true;
	});
	RegOP("not")
	.SetUnaryop([](AST::UnaryOp* op,AST::Value& R, AST::Value& v) {
		v = AST::Value(R.IsZero());
		return true;
	});
	RegOP("return")
	.SetUnaryop([](AST::UnaryOp* op,AST::Value& R, AST::Value& v) {
		AST::Scope* pScope = op->GetScope();
		if (pScope)
		{
			pScope->SetReturn(R);
		}
		v = R;
		return true;
	});
}
void Register()
{
	RegOP("False", "None", "True", "and", /*0-3*/
		"as", "assert", "async", "await", /*4-7*/
		"break", "class", "continue", /*8-10*/
		"def", "del", "elif", "else", /*11-14*/
		"except", "finally", "for", /*15-17*/
		"from", "global", "if", "import", /*18-21*/
		"in", "is", "lambda", "nonlocal", /*22-25*/
		"not", "or", "pass", "raise", "return", /*26-30*/
		"try", "while", "with", "yield"/*31-34*/);
	RegOP("range")
		.SetProcess(
			[](Parser* p, short opIndex) {
				auto op = new AST::Range(opIndex);
				return (AST::Operator*)op;
			});
	RegOP("return")
		.SetProcess([](Parser* p, short opIndex) {
			auto op = new AST::UnaryOp(opIndex);
			return (AST::Operator*)op;
		});
	RegOP("for")
		.SetProcess([](Parser* p, short opIndex){
			auto op = new AST::For(opIndex);
			return (AST::Operator*)op;
		});
	RegOP("while")
		.SetProcess([](Parser* p,short opIndex){
			auto op = new AST::While(opIndex);
			return (AST::Operator*)op;
		});
	RegOP("if", "elif")
		.SetProcess([](Parser* p, short opIndex){
			auto op = new AST::If(opIndex);
			return (AST::Operator*)op;
		});
	RegOP("else")
		.SetProcess([](Parser* p, short opIndex) {
		auto op = new AST::If(opIndex,false);
		return (AST::Operator*)op;
			});
	RegOP("in")
		.SetProcess([](Parser* p,short opIndex){
			auto op = new AST::InOp(opIndex);
			return (AST::Operator*)op;
		});
	RegOP("def")
		.SetProcess([](Parser* p, short opIndex){
			auto func = new AST::Func();
			return (AST::Operator*)func;
		});
	RegOP(//Python Assignment Operators
		"=", "+=", "-=", "*=", "/=", "%=", "//=",
		"**=", "&=", "|=", "^=", ">>=", "<<=")
		.SetProcess([](Parser* p, short opIndex){
			auto op = new AST::Assign(opIndex);
			return (AST::Operator*)op;
		})
		.SetPrecedence(Precedence_Min);

	RegOP(
		//Python Arithmetic Operators 
		"+", "-", "*", "/", "%", "**", "//")
	.SetProcess([](Parser* p, short opIndex){
			auto op = new AST::BinaryOp(opIndex);
			return (AST::Operator*)op;
	});

	RegOP(
		//Python Comparison Operators --index range[55,60]
		"==", "!=", ">", "<", ">=", "<=",
		//Python Logical  Operators
		"and", "or")
		.SetProcess([](Parser* p, short opIndex){
			auto op = new AST::BinaryOp(opIndex);
			return (AST::Operator*)op;
		});
	//set precedence just higher 1 with reqular
	RegOP("*", "/", "%", "**", "//")
		.SetPrecedence(Precedence_Reqular + 1);

	//Override for +-* which may be an unary Operator
	RegOP("+", "-", "*")
		.SetProcess([](Parser* p, short opIndex){
			AST::Operator* op = nil;
			if (p->PreTokenIsOp())
			{
				op = new AST::UnaryOp(opIndex);
			}
			else
			{
				op = new AST::BinaryOp(opIndex);
			}
			return op;
		});
	//Python Bitwise Operators --index range[61,66]
	RegOP("&", "|", "^", "~", "<<", ">>");
	RegOP("~", "not")
		.SetProcess([](Parser* p, short opIndex){
			AST::Operator* op = nil;
			if (p->PreTokenIsOp())
			{
				op = new AST::UnaryOp(opIndex);
			}
			else
			{
				//error
			}
			return op;
		});

	RegOP("(", "[", "{")
		.SetProcess([](Parser* p, short opIndex){
			return p->PairLeft(opIndex);
		});
	RegOP(")")
		.SetProcess([](Parser* p, short opIndex){
			p->PairRight(OP_ID::Parenthesis_L);
			return (AST::Operator*)nil;
		});
	RegOP("]")
		.SetProcess([](Parser* p, short opIndex){
			AST::Operator* op = nil;
			p->PairRight(OP_ID::Brackets_L);
			return op;
		});
	RegOP("}").SetProcess([](Parser* p, short opIndex)
		{
			p->PairRight(OP_ID::Curlybracket_L);
			return (AST::Operator*)nil;
		});
	RegOP(".").SetProcess([](Parser* p, short opIndex)
		{
			auto op = new AST::BinaryOp(opIndex);
			return (AST::Operator*)op;
		});
	RegOP(":").SetProcess([](Parser* p, short opIndex)
		{
			auto op = new AST::ColonOP(opIndex);
			return (AST::Operator*)op;
		});
	RegOP(",").SetProcess([](Parser* p, short opIndex)
		{
			auto op = new AST::CommaOp(opIndex);
			return (AST::Operator*)op;
		});
	//change precedence for ':'
	//OpInfo{{"in"},nil,Alias::In,Precedence_Reqular + 1},
	RegOP(":").SetPrecedence(Precedence_Reqular + 2);
	RegOP("\n").SetProcess([](Parser* p, short opIndex)
		{
			p->NewLine();
			return (AST::Operator*)nil;
		});

	RegOP("\t", "\r", "\\").SetProcess([](Parser* p, short opIndex)
		{
			auto op = new AST::Operator(opIndex);
			return op;
		});
	
	RegOP("(").SetId(OP_ID::Parenthesis_L);
	RegOP("[").SetId(OP_ID::Brackets_L);
	RegOP("{").SetId(OP_ID::Curlybracket_L);
	RegOP("\\").SetId(OP_ID::Slash);
	RegOP(":").SetId(OP_ID::Colon);
	RegOP("\t").SetId(OP_ID::Tab);
}

std::vector<OpInfo> RegOP::OPList;
void BuildOps()
{
	Register();
	RegisterOps();
	MakeLexTree(RegOP::OPList,G::I().GetKwTree(),
		G::I().GetOpActions());
}
RegOP& RegOP::SetId(OP_ID id)
{
	if (ops.size() > 0)
	{
		G::I().SetOpId(id, AddOrGet(ops[0]).id);
	}
	return *this;
}
}
