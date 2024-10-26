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
void X::Task::Cancel()
{
	if (m_taskPool.IsObject())
	{
		auto* pPool = dynamic_cast<X::Data::TaskPool*>(m_taskPool.GetObj());
		pPool->CancelTask(this);
	}
}
void X::Task::Cancelled()
{
	if (m_future.IsObject())
	{
		auto* pFuture = dynamic_cast<X::Data::Future*>(m_future.GetObj());
		pFuture->RemoveTask();
	}
}
void X::Task::run()
{
	SetStartRunTime(getCurMicroTimeStamp());
	m_valFunc.GetObj()->Call(m_rt, m_context.GetObj(), m_params,
		m_kwParams, m_retValue);
	SetEndRunTime(getCurMicroTimeStamp());

	if (m_future.IsObject())
	{
		auto* pFuture = dynamic_cast<X::Data::Future*>(m_future.GetObj());
		pFuture->SetVal(m_retValue);
		pFuture->RemoveTask();
	}
	delete this;
}
