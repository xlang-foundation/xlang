#include "event.h"
namespace X
{
	bool Event::Call(XRuntime* rt, XObj* pContext,
		ARGS& params,
		KWARGS& kwParams,
		X::Value& retValue)
	{
		DoFire(rt, pContext, params, kwParams);
		retValue = Value(true);
		return true;
	}
	void Event::FireInMain(X::XRuntime* rt, XObj* pContext,
		ARGS& params, KWARGS& kwargs)
	{
		EventSystem::I().FireInMain(this,rt,pContext,params,kwargs);
	}
}