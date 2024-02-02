#pragma once

//this module handle calling from Server side to Client
//need to quick response without blocking
//all server side calls to client will not return value
#include "singleton.h"
#include "locker.h"
#include "value.h"
#include "xhost.h"
#include <vector>

namespace X
{
	struct SrvCallInfo
	{
		X::Value ClientObj;//callable object such as function
		X::ARGS params;
		X::KWARGS kwParams;
	};
	class ServerCallPool :
		public Singleton<ServerCallPool>
	{
		//use to run code line for interactive mode
		std::vector<SrvCallInfo> m_CallPool;
		Locker m_lock;
		X::Value m_taskPool;
		void Init();

	public:
		void AddCall(SrvCallInfo& call);

	};

}

