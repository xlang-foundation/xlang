#include "pyproxyobject.h"
#include "utility.h"
#include "str.h"
#include "list.h"
#include "dict.h"
#include "xlang.h"
#include "bin.h"

namespace X
{
	namespace Data
	{
		#define PRELOAD_TAG "preload"

		//Shawn 8/4/2023, for embeded python, import cv2 has bugs,
		//can't import them in, but other modules like numpy are OK to import
		//workaround is preloading cv2.pyd from [python_folder]lib\site-packages\cv2\cv2.pyd
		// so we use this syntax to import cv2
		//  from preload import cv2
		//then in this GrusPyEngHost, will check from is equal to 'preload' or not
		//if it is, then find the native package's pyd file, load this lib
		// we think maybe this bug just in Windows
		//so just do in windows platform,for others, skip it

		PyProxyObject* PyObjectCache::QueryModule(std::string& fileName)
		{
			std::string strFileName = fileName;
			ReplaceAll(strFileName, "\\", "/");
			PyProxyObject* pRetObj = nullptr;
			auto it = m_mapModules.find(strFileName);
			if (it != m_mapModules.end())
			{
				pRetObj = it->second;
				pRetObj->Object::IncRef();
			}
			return pRetObj;
		}
		PyProxyObject::PyProxyObject(
			XlangRuntime* rt, XObj* pContext,
			std::string name, std::string fromPath,
			std::string curPath)
			:PyProxyObject()
		{
			static std::string preloadTag(PRELOAD_TAG);
			m_proxyType = PyProxyType::Module;
			m_name = name;
			m_path = curPath;
			//need to addRef()??
			//changed to IncRef for lock?? 2/2/2023
			Object::IncRef();
			if (rt->GetTrace())
			{
				rt->GetTrace()(rt, pContext, rt->GetCurrentStack(),
					TraceEvent::Call, m_pMyScope, this);
			}
			if (fromPath.empty())
			{
				std::string strFileName = GetPyModuleFileName();
				PyObjectCache::I().AddModule(strFileName, this);
				auto sys = PyEng::Object::Import("sys");
				sys["path.insert"](0, m_path);
				m_obj = g_pPyHost->Import(name.c_str());
				sys["path.remove"](m_path);
			}
			else if (fromPath == preloadTag)
			{
				m_obj = g_pPyHost->ImportWithPreloadRequired(name.c_str());
			}
			else
			{
				//here we need to change
				//for python: from module_name_here import sub1, sub2...
				//from part is the module, and import parts are subs inside
				//this module
				X::Port::vector<PyEngObjectPtr> subs(0);
				X::Port::vector<const char*> fromList(1);
				fromList.push_back(name.c_str());
				bool bOK = g_pPyHost->ImportWithFromList(fromPath.c_str(),
					fromList, subs);
				if (bOK && subs.size()>0)
				{
					m_obj = subs[0];
				}
			}
		}
		PyProxyObject::~PyProxyObject()
		{
			if (m_proxyType == PyProxyType::Module)
			{
				std::string strFileName = GetPyModuleFileName();
				PyObjectCache::I().RemoveModule(strFileName);
			}
		}
		bool PyProxyObject::ToValue(X::Value& val)
		{
			return PyObjectToValue(m_obj, val);
		}
		bool PyProxyObject::ToBin(X::Value& valBin)
		{
			return PyObjectToBin(m_obj, valBin);
		}
		bool PyProxyObject::GetItem(long long index, X::Value& val)
		{
			PyEng::Object subObj = m_obj[index];
			PyProxyObject* pProxyObj = new PyProxyObject(subObj);
			//todo: when to call release
			pProxyObj->Object::IncRef();
			val = X::Value(pProxyObj);
			return true;
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
				pProxyObj->Object::IncRef();
				val = X::Value(pProxyObj);
			}
			return true;
		}
		bool PyProxyObject::PyObjectToBin(PyEng::Object& pyObj, X::Value& valBin)
		{
			if (pyObj.IsArray()) //Numpy array to get its data and put data into Bin
			{
				char* pData = (char*)g_pPyHost->GetDataPtr(pyObj.ref());
				int itemType = 0;
				int itemSize = 0;
				X::Port::vector<unsigned long long> dims(0);
				X::Port::vector<unsigned long long> strides(0);
				g_pPyHost->GetDataDesc(pyObj.ref(), itemType, itemSize, dims, strides);
				//only convert one dimension array
				if (dims.size() == 1)
				{
					//calculate the total size
					//just one dimension array, so the size is the first dimension
					long long totalSize = dims[0]* itemSize;
					char* pBinData = new char[totalSize];
					memcpy(pBinData, pData, totalSize);
					//create a Bin object
					Data::Binary* pBin = new Data::Binary(pBinData, totalSize, true);
					valBin = X::Value(pBin);
				}
			}
			return true;
		}
		void PyProxyObject::EachVar(XlangRuntime* rt, XObj* pContext,
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
#if __TODO_SCOPE__
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
		int PyProxyObject::AddOrGet(std::string& name, bool bGetOnly, Scope** ppRightScope)
		{
			int idx = AST::Scope::AddOrGet(name, false);
			m_stackFrame->SetVarCount(GetVarNum());
			auto obj0 = (PyEng::Object)m_obj[name.c_str()];
			//check obj0 is a function or not

			PyProxyObject* pProxyObj = new PyProxyObject(m_obj,obj0,name);
			X::Value v(pProxyObj);
			m_stackFrame->Set(idx, v);
			return idx;
		}
#endif
		bool PyProxyObject::CalcCallables(XlangRuntime* rt, XObj* pContext,
			std::vector<AST::Scope*>& callables)
		{
			callables.push_back(dynamic_cast<AST::Scope*>(this));
			return true;
		}
		bool PyProxyObject::Call(XRuntime* rt, XObj* pContext,
			ARGS& params, KWARGS& kwParams,
			X::Value& retValue)
		{
			std::vector<X::Value> aryValues(params.Data(), params.Data() + params.size());
			PyEng::Tuple objParams(aryValues);
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
		List* PyProxyObject::FlatPack(XlangRuntime* rt, XObj* pContext,
			std::vector<std::string>& IdList, int id_offset,
			long long startIndex, long long count)
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
			if (pOutList)
			{
				pOutList->IncRef();
			}
			return pOutList;
		}
	}
}