#include "event.h"
namespace X
{
	bool ObjectEvent::Call(XRuntime* rt, XObj* pContext,
		ARGS& params,
		KWARGS& kwParams,
		X::Value& retValue)
	{
		DoFire(rt, pContext, params, kwParams);
		retValue = Value(true);
		return true;
	}
	void ObjectEvent::FireInMain(X::XRuntime* rt, XObj* pContext,
		ARGS& params, KWARGS& kwargs)
	{
		SetFire();
		EventSystem::I().FireInMain(this,rt,pContext,params,kwargs);
	}
}