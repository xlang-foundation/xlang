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