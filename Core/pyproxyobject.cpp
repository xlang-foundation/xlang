#include "pyproxyobject.h"

namespace X
{
	namespace Data
	{
		PyProxyObject::PyProxyObject(
			Runtime* rt, void* pContext,
			std::string name,
			std::string path)
			:PyProxyObject()
		{
			m_name = name;
			m_path = path;
			auto sys = PyEng::Object::Import("sys");
			sys["path.insert"](0, path);
			if (rt->GetTrace())
			{
				rt->GetTrace()(rt, pContext, rt->GetCurrentStack(),
					TraceEvent::Import, this, this);
			}
			m_obj = g_pHost->Import(name.c_str());
			sys["path.remove"](path);
		}
		int PyProxyObject::AddOrGet(std::string& name, bool bGetOnly)
		{
			int idx = AST::Scope::AddOrGet(name, false);
			m_stackFrame->SetVarCount(GetVarNum());
			auto obj0 = (PyEng::Object)m_obj[name.c_str()];
			PyProxyObject* pProxyObj = new PyProxyObject(obj0);
			AST::Value v(pProxyObj);
			m_stackFrame->Set(idx, v);
			return idx;
		}
		bool PyProxyObject::Call(Runtime* rt, 
			ARGS& params, KWARGS& kwParams,
			AST::Value& retValue)
		{
			PyEng::Tuple objParams(params);
			PyEng::Object objKwParams(kwParams);
			auto obj0 = (PyEng::Object)m_obj.Call(objParams, objKwParams);
			PyProxyObject* pProxyObj = new PyProxyObject(obj0);
			retValue = AST::Value(pProxyObj);
			return true;
		}
	}
}