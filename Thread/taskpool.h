#ifndef __taskpool_h__
#define __taskpool_h__

#include "object.h"
#include "gthread.h"
#include "wait.h"

namespace X
{
	class Task;
	namespace Data
	{
		class TaskPool;
		class WorkingThread:
			public GThread
		{
		private:
			bool m_run = true;
			bool m_isIdle = false;
			XWait* m_pWait = nullptr;
			TaskPool* m_pool = nullptr;
			// Inherited via GThread
			virtual void run() override;
		public:
			WorkingThread();
			~WorkingThread();
			inline void Stop()
			{
				m_run = false;
				m_pWait->Release();
				WaitToEnd();
			}
			inline void SetPool(TaskPool* p) { m_pool = p; }
			inline void ReleaseWait() { m_pWait->Release(); }
			inline bool IsIdle() { return m_isIdle; }
		};
		class TaskPool :
			virtual public Object
		{
		protected:
			std::vector<Task*> m_tasks;
			bool m_IsInUIThread = false;//Main Thread
			Locker m_lock;
			int m_nThreadNum = 10;
			std::vector<WorkingThread*> m_threads;
		public:
			TaskPool() :Object()
			{
				m_t = ObjType::TaskPool;
			}
			~TaskPool();
			void CancelAll();
			Task* GetTaskToRun();
			bool RunTask(Task* pTask);
			bool IsUIThread() { return m_IsInUIThread; }
			void SetInUIThread(bool b)
			{
				m_IsInUIThread = b;
			}
			void SetThreadNum(int num)
			{
				m_nThreadNum = num;
			}
		};
	}
}
#endif //__taskpool_h__