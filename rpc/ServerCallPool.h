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

#pragma once

//this module handle calling from Server side to Client
//need to quick response without blocking
//all server side calls to client will not return value
#include "singleton.h"
#include "Locker.h"
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

