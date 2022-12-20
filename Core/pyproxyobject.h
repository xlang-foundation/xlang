#pragma once
#include "object.h"
#include "PyEngObject.h"
#include "exp.h"
#include "value.h"
#include "runtime.h"
#include "stackframe.h"
#include "singleton.h"
#include <iostream>
namespace X
{
	namespace Data
	{
		class PyStackFrame :
			public AST::StackFrame
		{
			PyEng::Object m_frame;
		public:
			PyStackFrame()
			{
			}
			PyStackFrame(PyEngObjectPtr frm,AST::Scope* s)
			{
				m_frame = PyEng::Object(frm, true);
				m_pScope = s;
			}
		};
		enum PyProxyType
		{
			None,
			Module,
			Func,
			Line
		};
		class PyProxyObject;
		class PyObjectCache:
			public Singleton<PyObjectCache>
		{
			std::unordered_map<std::string, PyProxyObject*> m_mapModules;
		public:
			void AddModule(std::string& fileName, PyProxyObject* obj)
			{
				m_mapModules.emplace(std::make_pair(fileName,obj));
			}
			void RemoveModule(std::string& fileName)
			{
				auto it = m_mapModules.find(fileName);
				if (it != m_mapModules.end())
				{
					m_mapModules.erase(it);
				}
			}
			PyProxyObject* QueryModule(std::string& fileName);
		};
		struct iterator_info
		{
			PyEngObjectPtr iterator = nullptr;
			long long pos = 0;
		};
		//wrap for Python PyObject through PyEng::Object
		class PyProxyObject :
			public virtual Object,
			public virtual XPyObject,
			public virtual AST::Scope,
			public virtual AST::Expression
		{
			PyProxyObject* m_PyModule = nullptr;
			PyProxyType m_proxyType = PyProxyType::None;
			AST::StackFrame* m_stackFrame = nullptr;
			PyEng::Object m_obj;
			PyEng::Object m_pyFrameObject;
			std::string m_name;
			std::string m_path;
			std::string m_moduleFileName;//for func case,m_PyModule is null
			AST::Scope* m_myScope = nullptr;
			PyEng::Dict m_locals;
			PyEng::Dict m_globals;
		public:
			PyProxyObject():
				XPyObject(0), Object(), AST::Scope(), AST::Expression()
			{
				m_t = ObjType::PyProxyObject;
				m_stackFrame = new AST::StackFrame(this);
			}
			PyProxyObject(PyEng::Object& obj) :
				PyProxyObject()
			{
				m_obj = obj;
			}
			PyProxyObject(PyEng::Object& obj,std::string& name):
				PyProxyObject()
			{
				m_name = name;
				m_proxyType = PyProxyType::Func;
				m_obj = obj;
			}
			PyProxyObject(std::string ScopeName):
				PyProxyObject()
			{
				m_name = ScopeName;
				m_proxyType = PyProxyType::Func;
			}
			PyProxyObject(int line):
				PyProxyObject()
			{
				m_lineStart = line;
				m_proxyType = PyProxyType::Line;
			}
			PyProxyObject(std::string name, std::string path):
				PyProxyObject()
			{
				m_proxyType = PyProxyType::Module;
				m_name = name;
				m_path = path;
				std::string strFileName = GetModuleFileName();
				AddRef();
				PyObjectCache::I().AddModule(strFileName, this);
			}
			PyProxyObject(XlangRuntime* rt, XObj* pContext,
				std::string name,std::string fromPath,
				std::string curPath);
			virtual bool GetObj(void** ppObjPtr) override
			{
				*ppObjPtr = (PyEngObjectPtr)m_obj;
				return true;
			}
			void SetPyFrame(PyEng::Object objFrame)
			{
				m_pyFrameObject = objFrame;
			}
			bool GetItem(long long index, X::Value& val);
			inline bool MatchPyFrame(PyEngObjectPtr pyFrame)
			{
				return (m_pyFrameObject.ref() == pyFrame);
			}
			void SetLocGlob(PyEngObjectPtr lObj, PyEngObjectPtr gObj)
			{
				m_locals = PyEng::Object(lObj);
				m_globals = PyEng::Object(gObj);
			}
			void SetScope(AST::Scope* s)
			{
				m_myScope = s;
			}
			bool ToValue(X::Value& val);
			bool PyObjectToValue(PyEng::Object& pyObj, X::Value& val);
			virtual std::string GetNameString() override
			{
				return m_name;
			}
			inline virtual void CloseIterator(Iterator_Pos pos) override
			{
				iterator_info* pIterator_info = (iterator_info*)pos;
				if (pIterator_info)
				{
					if (pIterator_info->iterator)
					{
						g_pPyHost->Release(pIterator_info->iterator);
					}
					delete pIterator_info;
				}

			}
			inline virtual bool GetAndUpdatePos(Iterator_Pos& pos, ARGS& vals) override
			{
				iterator_info* pIterator_info = nullptr;
				if (pos == nullptr)
				{
					//pIterator_info will be closed by call CloseIterator
					//from outsite
					pIterator_info = new iterator_info({ nullptr,0 });
					pos = (Iterator_Pos)pIterator_info;
				}
				else
				{
					pIterator_info = (iterator_info*)pos;
				}
				if (m_obj.IsDict())
				{
					long long it = pIterator_info->pos;
					PyEngObjectPtr ptrKey = nullptr;
					PyEngObjectPtr ptrVal = nullptr;
					long long curIt = it;
					bool bOK = g_pPyHost->EnumDictItem(m_obj,it,ptrKey, ptrVal);
					if (bOK)
					{
						X::Value key, Val;
						PyEng::Object pyObjKey(ptrKey);
						PyEng::Object pyObjVal(ptrVal);
						PyObjectToValue(pyObjKey, key);
						PyObjectToValue(pyObjVal, Val);
						vals.push_back(key);
						vals.push_back(Val);
						vals.push_back(X::Value(curIt));
						pIterator_info->pos = it;
					}
					else
					{
						return false;
					}
				}
				else
				{
					//https://docs.python.org/3/c-api/iter.html#c.PyIter_Next
					PyEngObjectPtr iterator = nullptr;
					if (pIterator_info->iterator == nullptr)
					{
						iterator = g_pPyHost->GetIter(m_obj);
						pIterator_info->iterator = iterator;
					}
					else
					{
						iterator = pIterator_info->iterator;
					}
					PyEngObjectPtr pNext = g_pPyHost->GetIterNext(iterator);
					if (pNext == nullptr)
					{
						return false;
					}
					PyEng::Object itemObj = pNext;
					X::Value itemVal;
					PyObjectToValue(itemObj, itemVal);
					vals.push_back(itemVal);
					vals.push_back(X::Value(pIterator_info->pos));
					pIterator_info->pos++;
				}
				return true;
			}
			virtual void EachVar(XlangRuntime* rt, XObj* pContext,
				std::function<void(std::string, X::Value&)> const& f) override;
			virtual std::string GetModuleName(XlangRuntime* rt) override
			{
				if (m_proxyType == PyProxyType::Func)
				{
					return m_PyModule?m_PyModule->GetModuleFileName():
						m_moduleFileName;
				}
				else 
				{
					return GetModuleFileName();
				}
			}
			virtual Scope* GetScope() override
			{
				return m_myScope == nullptr?this: m_myScope;
			}
			virtual bool isEqual(Scope* s) override;
			virtual AST::ScopeWaitingStatus IsWaitForCall() override
			{
				return (m_proxyType == PyProxyType::Func 
					&& m_pyFrameObject.IsNull()) ||
					(m_proxyType == PyProxyType::Line)?
					AST::ScopeWaitingStatus::HasWaiting:
					AST::ScopeWaitingStatus::NoWaiting;
			}
			~PyProxyObject();
			void SetModule(PyProxyObject* pModule)
			{
				m_PyModule = pModule;
			}
			virtual void GetBaseScopes(std::vector<AST::Scope*>& bases) override
			{
				bases.push_back(dynamic_cast<Scope*>(this));
			}
			void SetModuleFileName(std::string& fileName)
			{
				m_moduleFileName = fileName;
			}
			std::string GetName()
			{
				return m_name;
			}
			std::string GetModuleFileName()
			{
				return m_path + "/" + m_name + ".py";
			}
			// Inherited via Scope
			virtual bool CalcCallables(XlangRuntime* rt, XObj* pContext,
				std::vector<AST::Scope*>& callables) override;
			virtual int AddOrGet(std::string& name, bool bGetOnly) override;
			virtual bool Set(XlangRuntime* rt, XObj* pContext, 
				int idx, X::Value& v) override
			{
				m_stackFrame->Set(idx, v);
				return true;
			}
			virtual bool Get(XlangRuntime* rt, XObj* pContext, 
				int idx, X::Value& v,
				X::LValue* lValue = nullptr) override
			{
				m_stackFrame->Get(idx, v, lValue);
				return true;
			}
			virtual Scope* GetParentScope() override
			{
				return nullptr;
			}
			virtual bool Call(XRuntime* rt, XObj* pContext, ARGS& params,
				KWARGS& kwParams, X::Value& retValue) override;
			virtual std::string ToString(bool WithFormat = false) override
			{
				return (std::string)m_obj;
			}
			virtual long long Size() override;
			virtual List* FlatPack(XlangRuntime* rt, XObj* pContext,
				std::vector<std::string>& IdList, int id_offset,
				long long startIndex, long long count) override;
		};
	}
}