#pragma once

#include "xpackage.h"
#include "xlang.h"

namespace X 
{
	class JsonWrapper
	{
		XPackageAPISet<JsonWrapper> m_Apis;
	public:
		XPackageAPISet<JsonWrapper>& APISET() { return m_Apis; }
	public:
		JsonWrapper()
		{
			m_Apis.AddFunc<1>("loads", &JsonWrapper::LoadFromString);
			m_Apis.AddRTFunc<1>("loads", &JsonWrapper::LoadFromFile);
			m_Apis.Create(this);
		}
		X::Value LoadFromString(std::string jsonStr);
		X::Value  LoadFromFile(X::XRuntime* rt, X::XObj* pContext,std::string fileName);
	};
}