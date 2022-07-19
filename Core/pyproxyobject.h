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
		//wrap for Python PyObject through PyEng::Object
		class PyProxyObject :
			public Object,
			public AST::Scope,
			public AST::Expression
		{
			PyProxyObject* m_PyModule = nullptr;
			PyProxyType m_proxyType = PyProxyType::None;
			AST::StackFrame* m_stackFrame = nullptr;
			PyEng::Object m_obj;
			std::string m_name;
			std::string m_path;
			std::string m_moduleFileName;//for func case,m_PyModule is null
			AST::Scope* m_myScope = nullptr;
			PyEng::Dict m_locals;
			PyEng::Dict m_globals;
		public:
			PyProxyObject()
			{
				m_t = Type::PyProxyObject;
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
			PyProxyObject(std::string ScopeName)
			{
				m_name = ScopeName;
				m_proxyType = PyProxyType::Func;
			}
			PyProxyObject(int line)
			{
				m_lineStart = line;
				m_proxyType = PyProxyType::Line;
			}
			PyProxyObject(std::string name, std::string path)
			{
				m_proxyType = PyProxyType::Module;
				m_name = name;
				m_path = path;
				std::string strFileName = GetModuleFileName();
				AddRef();
				PyObjectCache::I().AddModule(strFileName, this);
			}
			PyProxyObject(Runtime* rt, void* pContext,
				std::string name, std::string path);
			void SetLocGlob(PyEngObjectPtr lObj, PyEngObjectPtr gObj)
			{
				m_locals = PyEng::Object(lObj);
				/*
				auto keys = m_locals.Keys();
				for (int i = 0; i < keys.GetCount(); i++)
				{
					std::string name = (std::string)keys[i];
					AST::Scope::AddOrGet(name, false);
				}*/
				m_globals = PyEng::Object(gObj);
			}
			void SetScope(AST::Scope* s)
			{
				m_myScope = s;
			}
			virtual std::string GetNameString() override
			{
				return m_name;
			}
			virtual std::string GetModuleName(Runtime* rt) override
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
			virtual bool isProxyOf(Scope* s)
			{
				PyProxyObject* proxy_s = dynamic_cast<PyProxyObject*>(s);
				if (proxy_s)
				{
					if (m_proxyType == PyProxyType::Line &&
						proxy_s->m_proxyType == PyProxyType::Func)
					{
						return true;
					}
				}
				return false;
			}
			~PyProxyObject();
			void SetModule(PyProxyObject* pModule)
			{
				m_PyModule = pModule;
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
			virtual bool CalcCallables(Runtime* rt, void* pContext,
				std::vector<AST::Scope*>& callables) override;
			virtual int AddOrGet(std::string& name, bool bGetOnly) override;
			virtual bool Set(Runtime* rt, void* pContext, 
				int idx, AST::Value& v) override
			{
				m_stackFrame->Set(idx, v);
				return true;
			}
			virtual bool Get(Runtime* rt, void* pContext, 
				int idx, AST::Value& v,
				AST::LValue* lValue = nullptr) override
			{
				m_stackFrame->Get(idx, v, lValue);
				return true;
			}
			virtual Scope* GetParentScope() override
			{
				return nullptr;
			}
			virtual bool Call(Runtime* rt, ARGS& params,
				KWARGS& kwParams, AST::Value& retValue) override;
			virtual std::string ToString(bool WithFormat = false) override
			{
				return (std::string)m_obj;
			}
		};
	}
}