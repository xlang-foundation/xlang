#include "task.h"

void X::Task::run()
{
	m_pFunc->Call(m_pModule, m_pContext,false, m_params, m_kwParams, m_retValue);
}
