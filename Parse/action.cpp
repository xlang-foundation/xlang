﻿/*
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

#include "action.h"
#include "parser.h"
#include "exp.h"
#include "runtime.h"
#include "module.h"
#include "func.h"
#include "xclass.h"
#include "import.h"
#include "dotop.h"
#include "pipeop.h"
#include "lex.h"
#include "decor.h"
#include "op_registry.h"
#include "namespace_var.h"
#include "sql.h"
#include "await.h"
#include "jitblock.h"
#include "refop.h"

namespace X {

void RegisterOps(OpRegistry* reg)
{
	RegOP("+")
	.SetUnaryop([](XlangRuntime* rt,AST::UnaryOp* op,X::Value& R, X::Value& v) {
		v = R;//just keep as + does
		return true;
	})
	.SetBinaryop([](XlangRuntime* rt, AST::BinaryOp* op,X::Value& L, X::Value& R, X::Value& v) {
		//v = L;
		//v.Clone();
		v = L+R;
		return true;
	});
	RegOP("-")
	.SetUnaryop([](XlangRuntime* rt, AST::UnaryOp* op,X::Value& R, X::Value& v) {
		//set to 0
		if (R.GetType() == ValueType::Double)
		{
			v = X::Value(0.0);
		}
		else
		{
			v = X::Value(0);
		}
		v -= R;
		return true;
	})
	.SetBinaryop([](XlangRuntime* rt, AST::BinaryOp* op,X::Value& L, X::Value& R, X::Value& v) {
		//v = L;
		//v.Clone();
		//v -= R;
		v = L - R;
		return true;
	});
#if __not_tensor__
	RegOP("*")
	.SetBinaryop([](XlangRuntime* rt, AST::BinaryOp* op, X::Value& L, X::Value& R, X::Value& v) {
		v = L;
		v.Clone();
		v *= R;
		return true;
	});
#else
	RegOP("*")
		.SetBinaryop([](XlangRuntime* rt, AST::BinaryOp* op, X::Value& L, X::Value& R, X::Value& v) {
		v = L * R;
		return true;
	});
#endif
	RegOP(".")
	.SetBinaryop([](XlangRuntime* rt, AST::BinaryOp* op, X::Value& L, X::Value& R, X::Value& v) {
		int cnt = R.GetDigitNum();
		double d = (double)R.GetLongLong();
		for (int i = 0; i < cnt; i++)
		{
			d /= 10;
		}
		d += (double)L.GetLongLong();
		v = X::Value(d);
		return true;
	});
	RegOP("/")
	.SetBinaryop([](XlangRuntime* rt, AST::BinaryOp* op, X::Value& L, X::Value& R, X::Value& v) {
		v = L/R;
		return true;
	});
	RegOP("==")
	.SetBinaryop([](XlangRuntime* rt, AST::BinaryOp* op, X::Value& L, X::Value& R, X::Value& v) {
		v = X::Value(L == R);
		return true;
	});
	RegOP("!=")
	.SetBinaryop([](XlangRuntime* rt, AST::BinaryOp* op, X::Value& L, X::Value& R, X::Value& v) {
		v = X::Value(L != R);
		return true;
	});
	RegOP(">")
	.SetBinaryop([](XlangRuntime* rt, AST::BinaryOp* op, X::Value& L, X::Value& R, X::Value& v) {
		v = X::Value(L > R);
		return true;
	});
	RegOP("<")
	.SetBinaryop([](XlangRuntime* rt, AST::BinaryOp* op, X::Value& L, X::Value& R, X::Value& v) {
		v = X::Value(L < R);
		return true;
	});
	RegOP(">=")
	.SetBinaryop([](XlangRuntime* rt, AST::BinaryOp* op, X::Value& L, X::Value& R, X::Value& v) {
		v = X::Value(L >= R);
		return true;
	});
	RegOP("<=")
	.SetBinaryop([](XlangRuntime* rt, AST::BinaryOp* op, X::Value& L, X::Value& R, X::Value& v) {
		v = X::Value(L <= R);
		return true;
	});
	RegOP("and")
	.SetBinaryop([](XlangRuntime* rt, AST::BinaryOp* op, X::Value& L, X::Value& R, X::Value& v) {
		v = X::Value(L.IsTrue() && R.IsTrue());
		return true;
	});
	RegOP("or")
	.SetBinaryop([](XlangRuntime* rt, AST::BinaryOp* op, X::Value& L, X::Value& R, X::Value& v) {
		v = X::Value(L.IsTrue() || R.IsTrue());
		return true;
	});
	RegOP("not")
	.SetUnaryop([](XlangRuntime* rt, AST::UnaryOp* op,X::Value& R, X::Value& v) {
		v = X::Value(R.IsZero());
		return true;
	});
	RegOP("return")
	.SetUnaryop([](XlangRuntime* rt, AST::UnaryOp* op,
		X::Value& R, X::Value& v) {
		rt->SetReturn(R);
		v = R;
		return true;
	});
}
void Register(OpRegistry* reg)
{
	/*treat as Token 0-2*/
	RegOP("False", "True","None");
	RegOP("and","as", "assert", "async", "await",
		"break", "class", "continue",
		"def", "del", "elif", "else",
		"except", "finally", "for",
		"from", "global", "if", "import","thru",
		"in", "is", "lambda", "nonlocal",
		"const","var","namespace","|-",//same meaning
		"not", "or", "pass", "raise", "return",
		"try", "while", "with", "yield");
	RegOP("ref");//use to refernce a variable or expression
	RegOP("extern", "nonlocal","global")
		.SetProcess(
			[](Parser* p, short opIndex) {
				auto op = new AST::ExternDecl(opIndex);
				return (AST::Operator*)op;
			});
	RegOP("break","continue","pass")
		.SetProcess([](Parser* p, short opIndex) {
			auto op = new AST::ActionOperator(opIndex);
			return (AST::Operator*)op;
			});
	RegOP("return")
		.SetProcess([](Parser* p, short opIndex) {
			auto op = new AST::UnaryOp(opIndex);
			return (AST::Operator*)op;
		});
	RegOP("ref")
		.SetProcess([](Parser* p, short opIndex) {
		auto op = new AST::RefOp(opIndex);
		return (AST::Operator*)op;
		});
	RegOP("from")
		.SetProcess([](Parser* p, short opIndex) {
		auto op = new AST::From(opIndex);
		return (AST::Operator*)op;
			});
	RegOP("for")
		.SetProcess([](Parser* p, short opIndex){
			auto op = new AST::For(opIndex);
			return (AST::Operator*)op;
		});
	RegOP("await")
		.SetProcess([](Parser* p, short opIndex) {
		auto op = new AST::AwaitOp(opIndex);
		return (AST::Operator*)op;
			});
	RegOP("while")
		.SetProcess([](Parser* p,short opIndex){
			auto op = new AST::While(opIndex);
			return (AST::Operator*)op;
		});
	RegOP("if")
		.SetProcess([](Parser* p, short opIndex) {
			auto op = new AST::If(opIndex);
			op->SetFlag(true);
			return (AST::Operator*)op;
			});
	RegOP("elif")
		.SetProcess([](Parser* p, short opIndex){
			auto op = new AST::If(opIndex);
			op->SetFlag(false);
		return (AST::Operator*)op;
		});
	RegOP("else")
		.SetProcess([](Parser* p, short opIndex) {
			auto op = new AST::If(opIndex,false);
			op->SetFlag(false);
			return (AST::Operator*)op;
			});
	RegOP("in")
		.SetProcess([](Parser* p,short opIndex){
			auto op = new AST::InOp(opIndex);
			return (AST::Operator*)op;
		});
	RegOP("def","func","function")
		.SetProcess([](Parser* p, short opIndex){
		if (p->LastLineIsJitDecorator())
		{
			auto jitBlock = new AST::JitBlock(AST::JitType::Func);
			p->SetMeetJitBlock(true);
			return (AST::Operator*)jitBlock;
		}
		else
		{
			auto func = new AST::Func();
			return (AST::Operator*)func;
		}
		});
	RegOP("const", "var","namespace", "|-")
		.SetProcess([](Parser* p, short opIndex) {
		auto nmVar = new AST::NamespaceVar(opIndex);
		return (AST::Operator*)nmVar;
			});
	RegOP("class")
		.SetProcess([](Parser* p, short opIndex) {
		if (p->LastLineIsJitDecorator())
		{
			auto jitBlock = new AST::JitBlock(AST::JitType::Class);
			p->SetMeetJitBlock(true);
			return (AST::Operator*)jitBlock;
		}
		else
		{
			auto cls = new AST::XClass();
			return (AST::Operator*)cls;
		}
		});
	RegOP(//Python Assignment Operators
		"=", "+=", "-=", "*=", "/=", "%=", "//=",
		"**=", "&=", "|=", "^=", ">>=", "<<=")
		.SetProcess([](Parser* p, short opIndex) {
		auto op = new AST::Assign(opIndex);
		return (AST::Operator*)op;
		});
	RegOP(
		//Python Arithmetic Operators 
		"+", "-", "*", "/", "%", "**", "//")
	.SetProcess([](Parser* p, short opIndex){
			auto op = new AST::BinaryOp(opIndex);
			return (AST::Operator*)op;
	});
	RegOP("@").SetProcess([](Parser* p, short opIndex) {
		auto op = new AST::Decorator(opIndex);
		return (AST::Operator*)op;
	});
	//for import, from nnn as its left operand
	RegOP("import")
		.SetProcess([](Parser* p, short opIndex) {
		auto op = new AST::Import(opIndex);
		return (AST::Operator*)op;
			});
	RegOP("as")
		.SetProcess([](Parser* p, short opIndex) {
		auto op = new AST::AsOp(opIndex);
		return (AST::Operator*)op;
			});
	RegOP("deferred")
		.SetProcess([](Parser* p, short opIndex) {
		auto op = new AST::DeferredOP(opIndex);
		return (AST::Operator*)op;
			});
	RegOP("thru")
		.SetProcess([](Parser* p, short opIndex) {
		auto op = new AST::ThruOp(opIndex);
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
	RegOP("==", "!=", ">", "<", ">=", "<=","and","or").SetIds(reg,
		{ OP_ID::Equal,OP_ID::NotEqual,OP_ID::Great,OP_ID::Less,
		OP_ID::GreatAndEqual,OP_ID::LessAndEqual,OP_ID::And,OP_ID::Or});
	//for sql statment
#if ADD_SQL
	RegOP("SELECT")
		.SetProcess([](Parser* p, short opIndex) {
		p->SetSkipLineFeedFlags(true);//will set back to false after meet ;
		auto op = new AST::Select(opIndex);
	return (AST::Operator*)op;
		});
#endif
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
	RegOP("&", "|", "^", "~", "<<", ">>","->")
		.SetProcess([](Parser* p, short opIndex) 
	{
		auto op = new AST::BinaryOp(opIndex);
		return (AST::Operator*)op;
	});
	RegOP("->")
		.SetProcess([](Parser* p, short opIndex)
			{
				auto op = new AST::RetTypeOp(opIndex);
				return (AST::Operator*)op;
			});
	RegOP("|")
		.SetProcess([](Parser* p, short opIndex)
			{
				auto op = new AST::PipeOp(opIndex);
				return (AST::Operator*)op;
			});
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

	RegOP("(", "[", "{","<|")
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
	RegOP("|>")
		.SetProcess([](Parser* p, short opIndex) {
		AST::Operator* op = nil;
		p->PairRight(OP_ID::TableBracket_L);
		return op;
			});
	RegOP("}").SetProcess([](Parser* p, short opIndex)
		{
			p->PairRight(OP_ID::Curlybracket_L);
			return (AST::Operator*)nil;
		});
	RegOP(".").SetProcess([](Parser* p, short opIndex)
		{
			auto op = new AST::DotOp(opIndex,1);
			return (AST::Operator*)op;
		});
	RegOP("..").SetProcess([](Parser* p, short opIndex)
		{
			auto op = new AST::DotOp(opIndex,2);
			return (AST::Operator*)op;
		});
	RegOP("...").SetProcess([](Parser* p, short opIndex)
		{
			auto op = new AST::DotOp(opIndex, 3);
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
	RegOP(";").SetProcess([](Parser* p, short opIndex)
		{
			p->NewLine(false);
#if ADD_SQL
			//todo: need more check here
			p->SetSkipLineFeedFlags(false);
#endif
			return (AST::Operator*)nil;
		});
	RegOP("\n").SetProcess([](Parser* p, short opIndex)
		{
			p->NewLine(true);
			return (AST::Operator*)nil;
		});

	RegOP("\t", "\r", "\\").SetProcess([](Parser* p, short opIndex)
		{
			auto op = new AST::Operator(opIndex);
			return op;
		});

	RegOP("(").SetId(reg,OP_ID::Parenthesis_L);
	RegOP("<|").SetId(reg, OP_ID::TableBracket_L);
	RegOP("[").SetId(reg, OP_ID::Brackets_L);
	RegOP("{").SetId(reg, OP_ID::Curlybracket_L);
	RegOP("\\").SetId(reg, OP_ID::Slash);
	RegOP(":").SetId(reg, OP_ID::Colon);
	RegOP(",").SetId(reg, OP_ID::Comma);
	RegOP("\t").SetId(reg, OP_ID::Tab);
	RegOP("return").SetId(reg, OP_ID::ReturnOp);
	RegOP("break").SetId(reg, OP_ID::Break);
	RegOP("continue").SetId(reg, OP_ID::Continue);
	RegOP("pass").SetId(reg, OP_ID::Pass);

	//for Jit Func return type
	//for example: def Add_Two(m:int,n:int)->int:
	RegOP("->").SetId(reg, OP_ID::ReturnType);

	RegOP("=", "+=", "-=", "*=", "/=", "%=", "//=").SetIds(reg,
		{ OP_ID::Equ,OP_ID::AddEqu,OP_ID::MinusEqu,OP_ID::MulEqu,
		OP_ID::DivEqu,OP_ID::ModEqu,OP_ID::FloorDivEqu })
		.SetPrecedence(Precedence_Reqular-1);;
	RegOP("**=", "&=", "|=", "^=", ">>=", "<<=").SetIds(reg,
		{ OP_ID::PowerEqu,OP_ID::AndEqu,OP_ID::OrEqu,OP_ID::NotEqu,
		OP_ID::RightShiftEqu,OP_ID::LeftShitEqu })
		.SetPrecedence(Precedence_Reqular-1);

	//For jitblock
	RegOP("->")
		.SetPrecedence(Precedence_Reqular);

	//Calculation from left to right if same Precedence
	//so leading op such as if while for need to 
	//have lower Precedence as >,== etc.
	//todo: check other op also,
	RegOP("if","elif","else","while","for")
		.SetPrecedence(Precedence_Reqular-2);
	RegOP("and", "or")
		.SetPrecedence(Precedence_Reqular-1);
	//for example for in range(num), in needs have 
	//less Precedence than range which takes Precedence_Reqular
	RegOP("in")
		.SetPrecedence(Precedence_Reqular - 1);
	//
	RegOP("[", "]", "{", "}", "(",")")
		.SetPrecedence(Precedence_High);
	RegOP(".", "..", "...")
		.SetPrecedence(Precedence_High1);
	RegOP("const", "var","namespace", "|-")
		.SetPrecedence(Precedence_Reqular + 1);
	RegOP("*", "/", "%", "**", "//")
		.SetPrecedence(Precedence_Reqular + 1);
	RegOP("as")
		.SetPrecedence(Precedence_LOW2+1);
	RegOP("deferred")
		.SetPrecedence(Precedence_LOW2 + 1);
	RegOP("thru")
		.SetPrecedence(Precedence_LOW2-1);
	//comma set to Precedence_VERYLOW, 
	//for case import galaxy as t, earth as e
	//we need to make import has lower Precedence than comma
	RegOP("import")
		.SetPrecedence(Precedence_VERYLOW - 1);

	RegOP("extern", "nonlocal", "global")
		.SetPrecedence(Precedence_LOW2);

	RegOP("ref")
		.SetPrecedence(Precedence_Reqular);
#if ADD_SQL
	RegOP("SELECT")
		.SetPrecedence(Precedence_LOW1-1);
#endif
	//12/9/2022 todo: it was RegOP("\n",",",":")
	//but for def func1(x:int,y:double) case
	//need to make : at least has same Precedence as ','
	RegOP("\n",",")
		.SetPrecedence(Precedence_VERYLOW);
	RegOP("await")
		.SetPrecedence(Precedence_VERYVERYLOW);
	//for this case: t2 = t1[-20:120,-1:-3]
	//minus needs to be cacluated before :, so let : is below regular which minus op has that Precedence
	RegOP(":")
		.SetPrecedence(Precedence_VERYLOW+1);
}

std::vector<OpInfo> RegOP::OPList;
static OpRegistry __op_reg;
void BuildOps()
{
	//only need to run once
	static bool Inited = false;
	if (Inited)
	{
		return;
	}
	OpRegistry* reg = &__op_reg;
	G::I().SetReg(reg);
	Register(reg);
	RegisterOps(reg);
	Lex<OpInfo, OpAction>().MakeLexTree(
		RegOP::OPList,
		reg->GetKwTree(),
		reg->GetOpActions());
	reg->SetActionWithOpId();
	Inited = true;
}
RegOP& RegOP::SetId(OpRegistry* reg, OP_ID id)
{
	if (ops.size() > 0)
	{
		reg->SetOpId(id, AddOrGet(ops[0]).id);
	}
	return *this;
}
RegOP& RegOP::SetIds(OpRegistry* reg, std::vector<OP_ID> ids)
{
	for(int i=0;i<ops.size() && i<ids.size();i++)
	{
		reg->SetOpId(ids[i], AddOrGet(ops[i]).id);
	}
	return *this;
}
}
