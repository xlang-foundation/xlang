#include "task.h"
#include "runtime.h"
#include "func.h"

void X::Task::run()
{
	m_pFunc->Call(m_rt, m_pContext,m_params,
		m_kwParams, m_retValue);
}
