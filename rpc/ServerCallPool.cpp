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

#include "ServerCallPool.h"
#include "task.h"
#include "taskpool.h"
#include "function.h"


namespace X
{
	void ServerCallPool::Init()
	{
		X::Data::TaskPool* pPool = new X::Data::TaskPool();
		//pPool->SetThreadNum(num);
		m_taskPool = pPool;
	}
	//Create a Task in TaskPool,and pass in args and kwArgs
	//TaskPool will run the task in a working thread
	void ServerCallPool::AddCall(SrvCallInfo& call)
	{
		m_lock.Lock();
		if (m_taskPool.IsInvalid())
		{
			Init();
		}
		m_lock.Unlock();

		Task* pTask = new Task();
		pTask->SetTaskPool(m_taskPool);
		//Find client object's Runtime from it's Module
		XlangRuntime* rt_ClientObj = nullptr;
		auto* pXObj = call.ClientObj.GetObj();
		auto type = pXObj->GetType();
		switch (type)
		{
		case X::ObjType::Function:
		{
			auto* pObjFunc = dynamic_cast<X::Data::Function*>(pXObj);
			if (pObjFunc)
			{
				auto* pFunc = pObjFunc->GetFunc();
				if (pFunc)
				{
					auto* pModule = pFunc->GetMyModule();
					if (pModule)
					{
						rt_ClientObj = pModule->GetRT();
					}
				}
			}
		}
			break;
		default:
			break;
		}
		
		pTask->Call(call.ClientObj, rt_ClientObj,
			pXObj,
			call.params, call.kwParams);
	}
}
