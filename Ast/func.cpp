#include "func.h"
#include "object.h"
#include "function.h"
#include <string>
#include "glob.h"
#include "xclass_object.h"

namespace X
{
namespace AST
{
void Func::ScopeLayout()
{
	if (m_PassScopeLayout)
	{
		return;
	}
	m_PassScopeLayout = true;
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
			int idx = AddOrGet(strVarName, false);
			m_IndexofParamList.push_back(idx);
		}
		Params->ScopeLayout();
	}
}
std::string Func::getcode(bool includeHead)
{
	int startPos = m_charStart;
	int endPos = m_charEnd;
	int firstLineCharOff = 0;
	if (!includeHead && Body.size() >0)
	{
		auto& firstLine = Body[0];
		startPos = firstLine->GetCharStart();
		firstLineCharOff = firstLine->GetCharPos();
	}
	std::string code;
	Module* pMyModule = nil;
	Expression* pa = m_parent;
	while (pa != nil)
	{
		if (pa->m_type == ObType::Module)
		{
			pMyModule = dynamic_cast<Module*>(pa);
			break;
		}
		pa = pa->GetParent();
	}
	if (pMyModule)
	{
		code =  pMyModule->GetCodeFragment(startPos, endPos);
		if (!includeHead)
		{
			auto lines = split(code, '\n',false);
			code = "";
			if (lines.size() > 0)
			{
				code = lines[0];
			}
			for (int i=1;i<lines.size();i++)
			{
				auto& l = lines[i];
				code += '\n'+l.substr(firstLineCharOff);
			}
		}
	}
	return code;
}
bool Func::Exec(XlangRuntime* rt,ExecAction& action, XObj* pContext, Value& v, LValue* lValue)
{
	//when build from stream, n_Index stored into Stream,but m_scope is not
	if (m_Index == -1 || m_scope == nullptr)
	{
		ScopeLayout();
		//for lambda function, no name, so skip this check
		if (m_Name.size >0 && m_Index == -1)
		{
			return false;
		}
	}
	Data::Function* f = new Data::Function(this);
	//owned by Block
	Value v0(f);//hold one refcount
	XObj* pPassContext = f;
	for(auto it = m_decors.rbegin(); it != m_decors.rend(); ++it)
	{
		X::Value retVal;
		ExecAction action;
		(*it)->Exec(rt, action,pPassContext, retVal);
		if (retVal.IsObject())
		{
			pPassContext = retVal.GetObj();
			v0 = retVal;
		}
	}
	if (m_Index >= 0)
	{//Lambda doesn't need to register it, which doesn't have a name
		//TODO:11/16/2023
#if _TODO_
		m_scope->Set(rt, pContext, m_Index, v0);
#endif
	}
	v = v0;
	return true;
}
//this function seems just called from Decorator::Exec
//but maybe have some problems with remoting
//todo: need to check out
bool Func::CallEx(XRuntime* rt, XObj* pContext,
	ARGS& params,
	KWARGS& kwParams,
	X::Value& trailer,
	X::Value& retValue)
{
	kwParams.Add("origin", trailer);
	return Call(rt,pContext,params,kwParams,retValue);
}
Module* Func::GetMyModule()
{
	auto pa = m_parent;
	while (pa)
	{
		if (pa->m_type == ObType::Module)
		{
			return dynamic_cast<Module*>(pa);
		}
		else
		{
			pa = pa->GetParent();
		}
	}
	return nullptr;
}
bool Func::Call(XRuntime* rt0,
	XObj* pContext,
	ARGS& params,
	KWARGS& kwParams,
	Value& retValue)
{
	auto* rt_from = (XlangRuntime*)rt0;
	XlangRuntime* rt = G::I().Threading(rt_from);
	if (!rt->M())
	{
		rt->SetM(GetMyModule());
	}
	auto* pContextObj = dynamic_cast<X::Data::Object*>(pContext);
	StackFrame* pCurFrame = new StackFrame(m_pMyScope);
	for (auto& kw : kwParams)
	{
		std::string strKey(kw.key);
		m_pMyScope->AddOrGet(strKey, false);
	}
	rt->PushFrame(pCurFrame,m_pMyScope->GetVarNum());
	//for Class,Add this if This is not null
	if (m_IndexOfThis >=0 &&
		pContextObj && pContextObj->GetType() == X::ObjType::XClassObject)
	{
		Value v0(dynamic_cast<Data::Object*>(pContext));
		pCurFrame->Set(m_IndexOfThis, v0);
	}
	int num = (int)params.size();
	int indexNum = (int)m_IndexofParamList.size();
	if (num > indexNum)
	{
		num = indexNum;
	}
	for (int i = 0; i < num; i++)
	{
		pCurFrame->Set(m_IndexofParamList[i], params[i]);
	}
	for (auto& kw : kwParams)
	{
		std::string strKey(kw.key);
		int idx = m_pMyScope->AddOrGet(strKey, false);
		if (idx >= 0)
		{
			pCurFrame->Set(idx, kw.val);
		}
	}
	Value v0;
	ExecAction action;
	Block::Exec(rt,action,pContext, v0);
	rt->PopFrame();
	retValue = pCurFrame->GetReturnValue();
	delete pCurFrame;
	return true;
}
XObj* ExternFunc::GetRightContextForClass(XObj* pContext)
{
	auto* pXClassObject = dynamic_cast<Data::XClassObject*>(pContext);
	return pXClassObject->QueryBaseObjForPackage(m_pContext);
}
}
}