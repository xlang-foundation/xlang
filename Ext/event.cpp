#include "event.h"
namespace X
{
	bool ObjectEvent::Call(XRuntime* rt, XObj* pContext,
		ARGS& params,
		KWARGS& kwParams,
		X::Value& retValue)
	{
		auto* findSetValue = kwParams.find("SetValue");
		if (findSetValue != nullptr && params.size()>0)
		{
			*this += params[0];
			return true;
		}

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