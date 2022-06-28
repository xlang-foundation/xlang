#include "function.h"
#include "Locker.h"

namespace X
{
	namespace Data 
	{
		Function::Function(AST::Func* p)
		{
			m_t = Type::Function;
			m_func = p;
			m_lock = (void*)new Locker();
		}
		Function::~Function()
		{
			delete (Locker*)m_lock;
		}
		Runtime* Function::MakeThreadRuntime(long long curTId,Runtime* rt)
		{
			Runtime* pRet = nullptr;
			((Locker*)m_lock)->Lock();
			auto it = m_rtMap.find(curTId);
			if (it == m_rtMap.end())
			{
				X::Runtime* pRuntime = new X::Runtime();
				pRuntime->MirrorStacksFrom(rt);
				m_rtMap.emplace(std::make_pair(curTId, pRuntime));
				pRet = pRuntime;
			}
			else
			{
				pRet = it->second;
			}
			((Locker*)m_lock)->Unlock();
			return pRet;
		}
		bool Function::Call(Runtime* rt, ARGS& params,
			KWARGS& kwParams,
			AST::Value& retValue)
		{
			long long curTId = GetThreadID();
			if (rt->GetThreadId() != curTId)
			{
				rt = MakeThreadRuntime(curTId, rt);
			}
			return m_func->Call(rt, nullptr, params, kwParams, retValue);
		}
	}
}