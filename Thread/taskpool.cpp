#include "taskpool.h"
#include "task.h"
namespace X
{

	namespace Data
	{
		WorkingThread::WorkingThread()
		{
			m_pWait = new XWait();
		}
		WorkingThread::~WorkingThread()
		{
			delete m_pWait;
		}
		void WorkingThread::run()
		{
			while (m_run)
			{
				m_isIdle = false;
				Task* pTsk = m_pool->GetTaskToRun();
				while(pTsk)
				{
					pTsk->run();
					pTsk = m_pool->GetTaskToRun();
				}
				m_isIdle = true;
				m_pWait->Wait(-1);
			}
		}
		TaskPool::~TaskPool()
		{
			std::cout << "TaskPool::~TaskPool()";
			CancelAll();
		}

		void TaskPool::CancelAll()
		{
			m_lock.Lock();
			//if task inside m_tasks
			//it is not in any thread, so no WorkingThread holds this task
			//and can be deleted
			for (auto* pTsk : m_tasks)
			{
				delete pTsk;
			}
			m_tasks.clear();
			for (auto* pThread : m_threads)
			{
				pThread->Stop();
				delete pThread;
			}
			m_threads.clear();
			m_lock.Unlock();
		}
		Task* TaskPool::GetTaskToRun()
		{
			Task* pTsk = nullptr;
			m_lock.Lock();
			if (m_tasks.size() > 0)
			{
				pTsk = m_tasks[0];
				m_tasks.erase(m_tasks.begin());
			}
			m_lock.Unlock();
			return pTsk;
		}
		bool TaskPool::Call(XRuntime* rt, XObj* pContext,
			ARGS& params, KWARGS& kwParams, X::Value& retValue)
		{
			Task* pTsk = GetTaskToRun();
			while (pTsk)
			{
				pTsk->run();
				pTsk = GetTaskToRun();
			}
			return true;
		}
		bool TaskPool::RunTaskInUIThread(Task* pTask)
		{
			m_lock.Lock();
			m_tasks.push_back(pTask);
			m_lock.Unlock();
			UI_THREAD_RUN_HANDLER callHandler = g_pXHost->GetUIThreadRunHandler();
			if (callHandler)
			{
				X::Value callable(this);
				callHandler(callable, g_pXHost->GetUIThreadRunContext());
			}
			return true;
		}
		bool TaskPool::RunTask(Task* pTask)
		{
			if (m_IsInUIThread)
			{
				return RunTaskInUIThread(pTask);
			}
			m_lock.Lock();
			m_tasks.push_back(pTask);

			WorkingThread* pIdelThread = nullptr;
			for (auto* pThread : m_threads)
			{
				if (pThread->IsIdle())
				{
					pIdelThread = pThread;
					break;
				}
			}
			if (pIdelThread == nullptr && m_threads.size()< m_nThreadNum)
			{
				pIdelThread = new WorkingThread();
				pIdelThread->SetPool(this);
				m_threads.push_back(pIdelThread);
				pIdelThread->Start();
			}
			pIdelThread->ReleaseWait();
			m_lock.Unlock();
			return true;
		}
	}
}