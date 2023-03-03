#include "task.h"
#include "runtime.h"
#include "func.h"
#include "future.h"
#include "taskpool.h"

bool X::Task::Call(X::Value& valFunc,
	XlangRuntime* rt, XObj* pContext,
	ARGS& params,KWARGS& kwParams)
{
	m_valFunc = valFunc;
	m_context = pContext;
	m_params = params;
	m_kwParams = kwParams;

	//copy stacks into new thread
	X::XlangRuntime* pRuntime = new X::XlangRuntime();
	pRuntime->MirrorStacksFrom(rt);
	m_rt = pRuntime;
	
	if (m_taskPool.IsObject())
	{
		auto* pPool = dynamic_cast<X::Data::TaskPool*>(m_taskPool.GetObj());
		pPool->RunTask(this);
	}
	return true;
}
void X::Task::run()
{
	m_valFunc.GetObj()->Call(m_rt, m_context.GetObj(), m_params,
		m_kwParams, m_retValue);
	if (m_future.IsObject())
	{
		auto* pFuture = dynamic_cast<X::Data::Future*>(m_future.GetObj());
		pFuture->SetVal(m_retValue);
		pFuture->RemoveTask();
	}
	delete this;
}
