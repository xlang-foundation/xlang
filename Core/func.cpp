#include "func.h"
#include "object.h"

namespace X
{
namespace AST
{
void Func::ScopeLayout()
{
	static std::string THIS("this");
	Scope* pMyScope = GetScope();
	//for lambda function, no Name,so skip it when m_Name.size ==0
	if (pMyScope && m_Name.size > 0)
	{
		std::string strName(m_Name.s, m_Name.size);
		m_Index = pMyScope->AddOrGet(strName, false);
		//TODO: debug here
		if (m_parent->m_type == ObType::Class)
		{//it is class's member
			AddOrGet(THIS, false);
		}
	}
	//process parameters' default values
	if (Params)
	{
		auto& list = Params->GetList();
		m_positionParamCnt = (int)list.size();
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
bool Func::Run(Runtime* rt, void* pContext, Value& v, LValue* lValue)
{
	Data::Function* f = new Data::Function(this);
	Value v0(f);
	if (m_Index >= 0)
	{//Lambda doesn't need to register it, which doesn't have a name
		m_scope->Set(rt, pContext, m_Index, v0);
	}
	v = v0;
	return true;
}

bool Func::Call(Runtime* rt,
	void* pContext,
	std::vector<Value>& params,
	std::unordered_map<std::string, AST::Value>& kwParams,
	Value& retValue)
{
	static std::string THIS("this");
	auto* pContextObj = (X::Data::Object*)pContext;
	StackFrame* frame = new StackFrame(this);
	rt->PushFrame(frame,GetVarNum());
	//Add this if This is not null
	int pre_item = 0;
	if (pContextObj && pContextObj->GetType() == X::Data::Type::XClassObject)
	{
		int thisIdx = AddOrGet(THIS, true);
		Value v0(pContext);
		Scope::Set(rt, pContext, thisIdx, v0);
		pre_item++;
	}
	int num = m_positionParamCnt > (int)params.size() ?
		(int)params.size() : m_positionParamCnt;
	for (int i = 0; i < num; i++)
	{
		Scope::Set(rt, pContext, pre_item + i, params[i]);
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