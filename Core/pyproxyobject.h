/*
Copyright (C) 2024 The XLang Foundation
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

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
			public virtual AST::Expression,
			public virtual X::XCustomScope
		{
			PyProxyObject* m_PyModule = nullptr;
			PyProxyType m_proxyType = PyProxyType::None;
			AST::StackFrame* m_variableFrame = nullptr;
			PyEng::Object m_parent_obj;
			PyEng::Object m_obj;
			PyEng::Object m_pyFrameObject;
			std::string m_name;
			std::string m_path;
			std::string m_moduleFileName;//for func case,m_PyModule is null
			AST::Scope* m_pMyScopeProxy = nullptr;
			PyEng::Dict m_locals;
			PyEng::Dict m_globals;
		public:
			PyProxyObject():
				XPyObject(0), Object(),AST::Expression()
			{
				m_t = ObjType::PyProxyObject;

				m_pMyScopeProxy = new AST::Scope();
				m_pMyScopeProxy->SetType(AST::ScopeType::Custom);
				m_pMyScopeProxy->SetDynScope(static_cast<X::XCustomScope*>(this));


				m_variableFrame = new AST::StackFrame();
				m_pMyScope = new AST::Scope();
				m_pMyScope->SetType(AST::ScopeType::PyObject);
				m_pMyScope->SetVarFrame(m_variableFrame);

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
			PyProxyObject(PyEng::Object& parent,PyEng::Object& obj, std::string& name) :
				PyProxyObject()
			{
				m_name = name;
				m_parent_obj = parent;
				//todo: check here, it is not a func such as property
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
				std::string strFileName = GetPyModuleFileName();
				Object::IncRef();
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
			FORCE_INLINE bool MatchPyFrame(PyEngObjectPtr pyFrame)
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
				//todo: check here
				m_pMyScopeProxy = s;
			}
			virtual int cmp(X::Value* r) override
			{
				if (r->GetType() == ValueType::None && m_obj.IsNull())
				{
					return 0;
				}
				else if (r->GetType() == ValueType::Object 
					&& r->GetObj()->GetType() == X::ObjType::PyProxyObject)
				{
					PyProxyObject* pObj = dynamic_cast<PyProxyObject*>(r->GetObj());
					if (pObj)
					{
						if (pObj->m_obj.ref() == m_obj.ref())
						{
							return 0;
						}
					}
				}
				return 1;
			}
			bool ToValue(X::Value& val);
			bool ToBin(X::Value& valBin);
			static bool PyObjectToValue(PyEng::Object& pyObj, X::Value& val);
			static bool PyObjectToBin(PyEng::Object& pyObj, X::Value& valBin);

			virtual bool SupportAssign() override { return true; }
			virtual bool Assign(const X::Value& val) override
			{
				PyEng::Object newObj((X::Value&)val);
				if (m_parent_obj.ref() != nullptr)
				{
					m_parent_obj[m_name.c_str()] = newObj;
				}
				m_obj = newObj;
				return true;
			}
			FORCE_INLINE virtual void CloseIterator(Iterator_Pos pos) override
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
			FORCE_INLINE virtual bool GetAndUpdatePos(Iterator_Pos& pos,
				std::vector<Value>& vals,bool getOnly) override
			{
				//todo: deal with getOnly

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
				std::function<void(std::string, X::Value&)> const& f);
			virtual std::string GetModuleName(XlangRuntime* rt)
			{
				if (m_proxyType == PyProxyType::Func)
				{
					return m_PyModule?m_PyModule->GetPyModuleFileName():
						m_moduleFileName;
				}
				else 
				{
					return GetPyModuleFileName();
				}
			}
			virtual AST::Scope* GetMyScope() override
			{
				return m_pMyScopeProxy;
			}
#if __TODO_SCOPE__
			virtual Scope* GetScope() override
			{
				return m_pMyScopeProxy == nullptr?this: m_pMyScopeProxy;
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
#endif
			~PyProxyObject();
			void SetModule(PyProxyObject* pModule)
			{
				m_PyModule = pModule;
			}
			virtual void GetBaseScopes(std::vector<AST::Scope*>& bases) override
			{
				Object::GetBaseScopes(bases);
				bases.push_back(m_pMyScopeProxy);
			}
			void SetModuleFileName(std::string& fileName)
			{
				m_moduleFileName = fileName;
			}
			std::string GetName()
			{
				return m_name;
			}
			std::string GetPyModuleFileName()
			{
				if (m_path.empty()) return m_name + ".py";
				else return m_path + "/" + m_name + ".py";
			}
			virtual X::Value ToXlang() override;
			virtual bool CalcCallables(XlangRuntime* rt, XObj* pContext,
				std::vector<AST::Expression*> & callables) override; 

			// Inherited via DynamicScope
			virtual int AddOrGet(const char* name, bool bGetOnly) override;
			virtual bool Set(int idx, X::Value& v) override
			{
				m_variableFrame->Set(idx, v);
				return true;
			}
			virtual bool Get(int idx, X::Value& v, void* lValue = nullptr) override
			{
				m_variableFrame->Get(idx, v, (X::LValue*)lValue);
				return true;
			}

			virtual bool Call(XRuntime* rt, XObj* pContext, ARGS& params,
				KWARGS& kwParams, X::Value& retValue) override;
			virtual const char* ToString(bool WithFormat = false) override
			{
				std::string retStr = (std::string)m_obj;
				return GetABIString(retStr);
			}
			virtual long long Size() override;
			virtual List* FlatPack(XlangRuntime* rt, XObj* pContext,
				std::vector<std::string>& IdList, int id_offset,
				long long startIndex, long long count) override;
		};
	}
}