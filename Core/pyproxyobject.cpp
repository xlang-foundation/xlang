#include "pyproxyobject.h"
#include "utility.h"
#include "str.h"
#include "list.h"
#include "dict.h"

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
			Runtime* rt, XObj* pContext,
			std::string name, std::string fromPath,
			std::string curPath)
			:PyProxyObject()
		{
			m_proxyType = PyProxyType::Module;
			m_name = name;
			m_path = curPath;
			std::string strFileName = GetModuleFileName();
			//need to addRef()??
			AddRef();
			PyObjectCache::I().AddModule(strFileName, this);
			auto sys = PyEng::Object::Import("sys");
			sys["path.insert"](0, m_path);
			if (rt->GetTrace())
			{
				rt->GetTrace()(rt, pContext, rt->GetCurrentStack(),
					TraceEvent::Call, this, this);
			}
			m_obj = g_pPyHost->Import(name.c_str());
			sys["path.remove"](m_path);
		}
		PyProxyObject::~PyProxyObject()
		{
			if (m_proxyType == PyProxyType::Module)
			{
				std::string strFileName = GetModuleFileName();
				PyObjectCache::I().RemoveModule(strFileName);
			}
		}
		bool PyProxyObject::PyObjectToValue(PyEng::Object& pyObj, X::Value& val)
		{
			if (pyObj.IsBool())
			{
				val = X::Value((bool)pyObj);
			}
			else if (pyObj.IsLong())
			{
				val = X::Value((long long)pyObj);
			}
			else if (pyObj.IsDouble())
			{
				val = X::Value((double)pyObj);
			}
			else if (pyObj.IsString())
			{
				std::string strVal = (std::string)pyObj;
				Data::Str* pStrName = new Data::Str(strVal);
				val = X::Value(pStrName);
			}
			else
			{
				PyProxyObject* pProxyObj = new PyProxyObject(pyObj);
				//todo: when to call release
				pProxyObj->AddRef();
				val = X::Value(pProxyObj);
			}
			return true;
		}
		void PyProxyObject::EachVar(Runtime* rt, XObj* pContext,
			std::function<void(std::string, X::Value&)> const& f)
		{
			auto keys = m_locals.Keys();
			for (int i = 0; i < keys.GetCount(); i++)
			{
				std::string name = (std::string)keys[i];
				PyEng::Object objVal = (PyEng::Object)m_locals[name.c_str()];
				X::Value val;
				PyObjectToValue(objVal, val);
				f(name, val);
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
		bool PyProxyObject::CalcCallables(Runtime* rt, XObj* pContext,
			std::vector<AST::Scope*>& callables)
		{
			callables.push_back(dynamic_cast<AST::Scope*>(this));
			return true;
		}
		int PyProxyObject::AddOrGet(std::string& name, bool bGetOnly)
		{
			int idx = AST::Scope::AddOrGet(name, false);
			m_stackFrame->SetVarCount(GetVarNum());
			auto obj0 = (PyEng::Object)m_obj[name.c_str()];
			//check obj0 is a function or not

			PyProxyObject* pProxyObj = new PyProxyObject(obj0,name);
			X::Value v(pProxyObj);
			m_stackFrame->Set(idx, v);
			return idx;
		}
		bool PyProxyObject::Call(XRuntime* rt, XObj* pContext,
			ARGS& params, KWARGS& kwParams,
			X::Value& retValue)
		{
			PyEng::Tuple objParams(params);
			PyEng::Object objKwParams(kwParams);
			auto obj0 = (PyEng::Object)m_obj.Call(objParams, objKwParams);
			PyProxyObject* pProxyObj = new PyProxyObject(obj0);
			retValue = X::Value(pProxyObj);
			return true;
		}
		long long PyProxyObject::Size()
		{
			if (m_obj.IsBool() || m_obj.IsLong() ||
				m_obj.IsDouble() || m_obj.IsString())
			{
				return 0;
			}
			return m_obj.GetCount();
		}
		List* PyProxyObject::FlatPack(Runtime* rt, long long startIndex,
			long long count)
		{
			List* pOutList = nullptr;
			if (m_obj.IsDict())
			{
				long long pos = 0;
				PyEng::Object objKey, objVal;
				PyEng::Dict pyDict(m_obj);
				pOutList = new List();
				while (pyDict.Enum(pos, objKey, objVal))
				{
					Dict* dict = new Dict();
					X::Value key, val;
					PyObjectToValue(objKey, key);
					if (!key.IsObject() 
						|| (key.IsObject() && 
							dynamic_cast<Object*>(key.GetObj())->IsStr()))
					{
						dict->Set("Name", key);
					}
					else if (key.IsObject())
					{
						X::Value objId((unsigned long long)key.GetObj());
						dict->Set("Name", objId);
					}
					PyObjectToValue(objVal, val);
					auto valType = val.GetValueType();
					Data::Str* pStrType = new Data::Str(valType);
					dict->Set("Type", X::Value(pStrType));
					if (!val.IsObject() || (val.IsObject() && 
						dynamic_cast<Object*>(val.GetObj())->IsStr()))
					{
						dict->Set("Value", val);
					}
					else if (val.IsObject())
					{
						X::Value objId((unsigned long long)val.GetObj());
						dict->Set("Value", objId);
						X::Value valSize(val.GetObj()->Size());
						dict->Set("Size", valSize);
					}
					X::Value valDict(dict);
					pOutList->Add(rt, valDict);
				}
			}
			else if (m_obj.IsArray())
			{

			}
			else
			{
				auto itemCnt = m_obj.GetCount();
				if (startIndex < 0 || startIndex >= itemCnt)
				{
					return nullptr;
				}
				if (count == -1)
				{
					count = itemCnt - startIndex;
				}
				if ((startIndex + count) > Size())
				{
					return nullptr;
				}
				pOutList = new List();
				for (long long i = 0; i < count; i++)
				{
					long long idx = startIndex + i;
					PyEng::Object objVal = m_obj[(int)i];
					X::Value val;
					PyObjectToValue(objVal, val);
					Dict* dict = new Dict();
					auto valType = val.GetValueType();
					Data::Str* pStrType = new Data::Str(valType);
					dict->Set("Type", X::Value(pStrType));
					if (!val.IsObject() || (val.IsObject() && 
						dynamic_cast<Object*>(val.GetObj())->IsStr()))
					{
						dict->Set("Value", val);
					}
					else if (val.IsObject())
					{
						X::Value objId((unsigned long long)val.GetObj());
						dict->Set("Value", objId);
						X::Value valSize(val.GetObj()->Size());
						dict->Set("Size", valSize);
					}
					X::Value valDict(dict);
					pOutList->Add(rt, valDict);
				}
			}
			return pOutList;
		}
	}
}