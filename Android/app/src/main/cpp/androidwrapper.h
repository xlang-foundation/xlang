#pragma once

#include "xpackage.h"
#include "xlang.h"
#include <jni.h>

namespace X
{
	class AndroidWrapper
	{
		JNIEnv* m_env = nullptr;
		jobject m_objHost = nullptr;
	public:
		BEGIN_PACKAGE(AndroidWrapper)
			APISET().AddFunc<1>("print", &AndroidWrapper::Print);
		END_PACKAGE
	public:
		AndroidWrapper()
		{

		}
		AndroidWrapper(JNIEnv* env, jobject objHost)
		{
			m_env = env;
			m_objHost = objHost;
		}
		~AndroidWrapper()
		{
			if(m_env != nullptr && m_objHost!= nullptr)
			{
				m_env->DeleteGlobalRef(m_objHost);
			}
		}
		bool Print(std::string info);
	};
}