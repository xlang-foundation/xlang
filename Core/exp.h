#pragma once

#include "pycore.h"

namespace XPython {namespace AST{
class Expression
{

};
class Assign:
	public Expression
{
	short Op;//index of _kws for Assignment Operators
	Expression* L;
	Expression* Val;
};

class BinaryOp :
	public Expression
{
	short Op;//index of _kws for Arithmetic && Comparison Operators
	Expression* L;
	Expression* R;
};


class Var:
	public Expression
{
	String Name;

};
class Number :
	public Expression
{

};
class Double :
	public Expression
{

};

}}