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
#include "singleton.h"
#include <string>
#include <unordered_map>
#include <vector>
#include "utility.h"
#include "runtime.h"
#include <iostream>
#include "Locker.h"
#include "stackframe.h"

namespace X {
	namespace Data { class Object; }
	namespace AST { class StackFrame; }
	class OpRegistry;
	class XlangRuntime;
	class G:
		public Singleton<G>
	{
		struct ObjInfo
		{
#if XLANG_ENG_DBG
			std::vector<AST::StackFrame*> stacksOwn;
#endif
		};
		std::unordered_map<long long, XlangRuntime*> m_rtMap;//for multiple threads
		void* m_lockRTMap = nullptr;
		XlangRuntime* MakeThreadRuntime(std::string& name,long long curTId, XlangRuntime* rt);
		std::unordered_map<Data::Object*, ObjInfo> Objects;
		void* m_lock = nullptr;
		OpRegistry* m_reg = nullptr;
		void notityThread(const char* strType, int tid);
		void* m_lockBreakpointsMap = nullptr;
		std::unordered_map<std::string, std::vector<int>> m_srcPathBreakpointsMap;// 
		std::unordered_map<std::string, std::vector<int>> m_srcMd5BreakpointsMap;
		void* m_lockBreakpointsValid = nullptr;
		std::vector<std::string> m_srcPathBreakpointsValid;
		std::vector<std::string> m_srcMd5BreakpointsValid;
		XTraceFunc m_tracefunc = nullptr;
	public:
		G();
		~G();
		FORCE_INLINE void SetTrace(XTraceFunc f){m_tracefunc = f; }
		FORCE_INLINE XTraceFunc GetTrace() { return m_tracefunc; }
		void BindRuntimeToThread(XlangRuntime* rt);
		void UnbindRuntimeToThread(XlangRuntime* rt);
		FORCE_INLINE OpRegistry& R() { return *m_reg;}
		FORCE_INLINE void SetReg(OpRegistry* r) { m_reg = r; }
		FORCE_INLINE std::unordered_map<long long, XlangRuntime*> GetThreadRuntimeIdMap() 
		{
			((Locker*)m_lockRTMap)->Lock();
			std::unordered_map<long long, XlangRuntime*> ret(m_rtMap);
			((Locker*)m_lockRTMap)->Unlock();
			return  ret; 
		}
		FORCE_INLINE AST::Module* QueryModuleByThreadId(int threadId)
		{
			AST::Module* ret = nullptr;
			((Locker*)m_lockRTMap)->Lock();
			auto it = m_rtMap.find(threadId);
			if (it != m_rtMap.end())
			{
				ret = it->second->M();
			}
			((Locker*)m_lockRTMap)->Unlock();
			return  ret;
		}
		FORCE_INLINE XRuntime* QueryRuntimeForThreadId(unsigned long threadId)
		{
			XRuntime* pRet = nullptr;
			((Locker*)m_lockRTMap)->Lock();
			auto it = m_rtMap.find(threadId);
			if (it != m_rtMap.end())
			{
				pRet = dynamic_cast<XRuntime*>(it->second);
			}
			((Locker*)m_lockRTMap)->Unlock();
			return pRet;
		}
		FORCE_INLINE XRuntime* GetCurrentRuntime(XRuntime* baseRt = nullptr)
		{
			unsigned long tid = GetThreadID();
			XRuntime* pRet = nullptr;
			((Locker*)m_lockRTMap)->Lock();
			auto it = m_rtMap.find(tid);
			if (it != m_rtMap.end())
			{
				pRet = dynamic_cast<XRuntime*>(it->second);
			}
			((Locker*)m_lockRTMap)->Unlock();
			if (pRet == nullptr)
			{
				//TODO:
				std::string defName;
				pRet =  MakeThreadRuntime(defName, tid,baseRt?dynamic_cast<XlangRuntime*>(baseRt):nullptr);
			}
			return pRet;
		}
		FORCE_INLINE XlangRuntime* Threading(std::string& name,XlangRuntime* fromRt)
		{
			long long curTId = GetThreadID();
			if (fromRt == nullptr || fromRt->GetThreadId() != curTId)
			{
				fromRt = MakeThreadRuntime(name,curTId, fromRt);
			}
			return fromRt;
		}

		FORCE_INLINE void SetBreakPoints(const std::string& path, std::vector<int> breakPoints)
		{
			((Locker*)m_lockBreakpointsMap)->Lock();
			m_srcPathBreakpointsMap[path] = breakPoints;
			((Locker*)m_lockBreakpointsMap)->Unlock();
		}

		FORCE_INLINE std::vector<int> GetBreakPoints(const std::string& path)
		{
			std::vector<int> points;
			((Locker*)m_lockBreakpointsMap)->Lock();
			auto it = m_srcPathBreakpointsMap.find(path);
			if (it != m_srcPathBreakpointsMap.end())
				points = it->second;
			((Locker*)m_lockBreakpointsMap)->Unlock();

			return points;
		}

		FORCE_INLINE void SetBreakPointsMd5(const std::string& strMd5, std::vector<int> breakPoints)
		{
			((Locker*)m_lockBreakpointsMap)->Lock();
			m_srcMd5BreakpointsMap[strMd5] = breakPoints;
			((Locker*)m_lockBreakpointsMap)->Unlock();
		}
		FORCE_INLINE std::vector<int> GetBreakPointsMd5(const std::string& strMd5)
		{
			std::vector<int> points;
			((Locker*)m_lockBreakpointsMap)->Lock();
			auto it = m_srcMd5BreakpointsMap.find(strMd5);
			if (it != m_srcMd5BreakpointsMap.end())
				points = it->second;
			((Locker*)m_lockBreakpointsMap)->Unlock();

			return points;
		}

		FORCE_INLINE bool IsBreakpointValid(const std::string& path)
		{
			bool ret;
			((Locker*)m_lockBreakpointsMap)->Lock();
			ret = std::find(m_srcPathBreakpointsValid.begin(), m_srcPathBreakpointsValid.end(),path) != m_srcPathBreakpointsValid.end();
			((Locker*)m_lockBreakpointsMap)->Unlock();
			return ret;
		}

		FORCE_INLINE void AddBreakpointValid(const std::string& path)
		{
			((Locker*)m_lockBreakpointsMap)->Lock();
			if(std::find(m_srcPathBreakpointsValid.begin(), m_srcPathBreakpointsValid.end(), path) == m_srcPathBreakpointsValid.end())
				m_srcPathBreakpointsValid.push_back(path);
			((Locker*)m_lockBreakpointsMap)->Unlock();
		}

		FORCE_INLINE bool IsBreakpointValidMd5(const std::string& strMd5)
		{
			bool ret;
			((Locker*)m_lockBreakpointsMap)->Lock();
			ret = std::find(m_srcMd5BreakpointsValid.begin(), m_srcMd5BreakpointsValid.end(), strMd5) != m_srcMd5BreakpointsValid.end();
			((Locker*)m_lockBreakpointsMap)->Unlock();
			return ret;
		}

		FORCE_INLINE void AddBreakpointValidMd5(const std::string& strMd5)
		{
			((Locker*)m_lockBreakpointsMap)->Lock();
			if (std::find(m_srcMd5BreakpointsValid.begin(), m_srcMd5BreakpointsValid.end(), strMd5) == m_srcMd5BreakpointsValid.end())
				m_srcMd5BreakpointsValid.push_back(strMd5);
			((Locker*)m_lockBreakpointsMap)->Unlock();
		}

		void Lock();
		void UnLock();
		void Check()
		{
			Lock();
			auto size = Objects.size();
			UnLock();
			//std::cout << "Left Objects:" << size << std::endl;
		}
#if XLANG_ENG_DBG
		void ObjBindToStack(XObj* pXObj, AST::StackFrame* pStack);
		void ObjUnbindToStack(XObj* pXObj, AST::StackFrame* pStack);
#endif
		FORCE_INLINE void AddObj(Data::Object* obj)
		{
			Lock();
			Objects.emplace(std::make_pair(obj,ObjInfo{}));
			UnLock();
		}
		FORCE_INLINE void RemoveObj(Data::Object* obj)
		{
			Lock();
			auto it = Objects.find(obj);
			if (it != Objects.end())
			{
#if XLANG_ENG_DBG
				if (it->second.stacksOwn.size() > 0)
				{
					std::cout << "Object still have ref in stack" 
						<< it->second.stacksOwn.size() << std::endl;
				}
#endif
				Objects.erase(it);
			}
			UnLock();
		}
	};
}