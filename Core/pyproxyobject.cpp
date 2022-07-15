#include "pyproxyobject.h"

namespace X
{
	namespace Data
	{
		PyProxyObject::PyProxyObject(std::string name, std::string path)
			:PyProxyObject()
		{
			auto sys = PyEng::Object::Import("sys");
			sys["path.insert"](0, path);
			m_obj = g_pHost->Import(name.c_str());
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