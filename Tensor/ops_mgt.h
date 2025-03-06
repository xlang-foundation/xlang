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

#include "singleton.h"


namespace X
{
	namespace Data
	{
		//manage ops per thread
		//only used for symbol operators such as * / + - etc.
		//and all other named operators use name directly inside expresion, 
		//so will create TensorOperator object as return
		//each thread will keep a stack for ops, because there are many diffrent impl. of 
		//ops libs, in same thread, some place use lib A(CPU) and another place use lib B(Cuda)
		
		class OpsManager :
			public Singleton<OpsManager>
		{
			int m_TempTensorLastIndex = 100;
		public:
			std::string GenNewName()
			{
				int idx=  m_TempTensorLastIndex++;
				char v[1000];
				snprintf(v, sizeof(v), "temp_%d",idx);
				return v;
			}
		};
	}
}