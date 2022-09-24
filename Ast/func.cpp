#include "func.h"
#include "object.h"
#include "function.h"
#include <string>
#include "glob.h"

namespace X
{
namespace AST
{
void Func::ScopeLayout()
{
	std::string thisKey("this");
	Scope* pMyScope = GetScope();
	//for lambda function, no Name,so skip it when m_Name.size ==0
	if (pMyScope && m_Name.size > 0)
	{
		std::string strName(m_Name.s, m_Name.size);
		m_Index = pMyScope->AddOrGet(strName, false);
		//TODO: debug here
		if (m_parent->m_type == ObType::Class)
		{//it is class's member
			m_IndexOfThis = AddOrGet(thisKey, false);
		}
	}
	//process parameters' default values
	if (Params)
	{
		auto& list = Params->GetList();
		m_positionParamCnt = (int)list.size();
		m_paramStartIndex = Scope::GetVarNum();//if some vars already pushed
		//this case happened in lambda function
		for (auto i : list)
		{
			std::string strVarName;
			std::string strVarType;
			Value defaultValue;
			switch (i->m_type)
			{
			case ObType::Var:
			{
				Var* varName = dynamic_cast<Var*>(i);
				String& szName = varName->GetName();
				strVarName = std::string(szName.s, szName.size);
			}
			break;
			case ObType::Assign:
			{
				Assign* assign = dynamic_cast<Assign*>(i);
				Var* varName = dynamic_cast<Var*>(assign->GetL());
				String& szName = varName->GetName();
				strVarName = std::string(szName.s, szName.size);
				Expression* defVal = assign->GetR();
				auto* pExprForDefVal = new Data::Expr(defVal);
				defaultValue = Value(pExprForDefVal);
			}
			break;
			case ObType::Param:
			{
				Param* param = dynamic_cast<Param*>(i);
				param->Parse(strVarName, strVarType, defaultValue);
			}
			break;
			}
			AddOrGet(strVarName, false);
		}
	}
}
bool Func::Run(Runtime* rt, XObj* pContext, Value& v, LValue* lValue)
{
	Data::Function* f = new Data::Function(this);
	Value v0(f);//hold one refcount
	XObj* pPassContext = f;
	for(auto it = m_decors.rbegin(); it != m_decors.rend(); ++it)
	{
		X::Value retVal;
		(*it)->Run(rt, pPassContext, retVal);
		if (retVal.IsObject())
		{
			pPassContext = retVal.GetObj();
			v0 = retVal;
		}
	}
	if (m_Index >= 0)
	{//Lambda doesn't need to register it, which doesn't have a name
		m_scope->Set(rt, pContext, m_Index, v0);
	}
	v = v0;
	return true;
}
bool Func::CallEx(XRuntime* rt, XObj* pContext,
	ARGS& params,
	KWARGS& kwParams,
	X::Value& trailer,
	X::Value& retValue)
{
	return true;
}
bool Func::Call(XRuntime* rt0,
	XObj* pContext,
	std::vector<Value>& params,
	KWARGS& kwParams,
	Value& retValue)
{
	Runtime* rt = G::I().Threading((Runtime*)rt0);
	auto* pContextObj = dynamic_cast<X::Data::Object*>(pContext);
	StackFrame* frame = new StackFrame(this);
	for (auto& kw : kwParams)
	{
		Scope::AddOrGet((std::string&)kw.first, false);
	}
	rt->PushFrame(frame,GetVarNum());
	//Add this if This is not null
	if (m_IndexOfThis >=0 &&
		pContextObj && pContextObj->GetType() == X::ObjType::XClassObject)
	{
		Value v0(dynamic_cast<Data::Object*>(pContext));
		Scope::Set(rt, pContext, m_IndexOfThis, v0);
	}
	int num = m_positionParamCnt > (int)params.size() ?
		(int)params.size() : m_positionParamCnt;
	for (int i = 0; i < num; i++)
	{
		Scope::Set(rt, pContext, m_paramStartIndex + i, params[i]);
	}
	for (auto& kw : kwParams)
	{
		int idx = Scope::AddOrGet((std::string&)kw.first, false);
		if (idx >= 0)
		{
			Scope::Set(rt, pContext, idx, kw.second);
		}
	}
	Value v0;
	Block::Run(rt, pContext, v0);
	rt->PopFrame();
	retValue = frame->GetReturnValue();
	delete frame;
	return true;
}
}
}