#include "pyproxyobject.h"
#include "utility.h"

namespace X
{
	namespace Data
	{
		PyProxyObject* PyObjectCache::QueryModule(std::string& fileName)
		{
			std::string strFileName = fileName;
			ReplaceAll(strFileName, "\\", "/");
			PyProxyObject* pRetObj = nullptr;
			auto it = m_mapModules.find(strFileName);
			if (it != m_mapModules.end())
			{
				pRetObj = it->second;
				pRetObj->AddRef();
			}
			return pRetObj;
		}
		PyProxyObject::PyProxyObject(
			Runtime* rt, void* pContext,
			std::string name,
			std::string path)
			:PyProxyObject()
		{
			m_proxyType = PyProxyType::Module;
			m_name = name;
			m_path = path;
			std::string strFileName = GetModuleFileName();
			//need to addRef()??
			AddRef();
			PyObjectCache::I().AddModule(strFileName, this);
			auto sys = PyEng::Object::Import("sys");
			sys["path.insert"](0, path);
			if (rt->GetTrace())
			{
				rt->GetTrace()(rt, pContext, rt->GetCurrentStack(),
					TraceEvent::Call, this, this);
			}
			m_obj = g_pHost->Import(name.c_str());
			sys["path.remove"](path);
		}
		PyProxyObject::~PyProxyObject()
		{
			if (m_proxyType == PyProxyType::Module)
			{
				std::string strFileName = GetModuleFileName();
				PyObjectCache::I().RemoveModule(strFileName);
			}
		}
		bool PyProxyObject::isEqual(Scope* s)
		{
			PyProxyObject* pS_Proxy = dynamic_cast<PyProxyObject*>(s);
			if (pS_Proxy == nullptr)
			{
				return false;
			}
			return (pS_Proxy->m_name == m_name &&
				pS_Proxy->m_proxyType == m_proxyType);
		}
		bool PyProxyObject::CalcCallables(Runtime* rt, void* pContext,
			std::vector<AST::Scope*>& callables)
		{
			if (m_proxyType == PyProxyType::Module ||
				m_proxyType == PyProxyType::Func)
			{
				callables.push_back(dynamic_cast<AST::Scope*>(this));
			}
			return true;
		}
		int PyProxyObject::AddOrGet(std::string& name, bool bGetOnly)
		{
			int idx = AST::Scope::AddOrGet(name, false);
			m_stackFrame->SetVarCount(GetVarNum());
			auto obj0 = (PyEng::Object)m_obj[name.c_str()];
			//check obj0 is a function or not

			PyProxyObject* pProxyObj = new PyProxyObject(obj0,name);
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