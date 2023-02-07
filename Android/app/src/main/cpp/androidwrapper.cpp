#include "androidwrapper.h"

namespace X
{
	bool AndroidWrapper::Print(std::string info)
	{
		jclass objClass = m_env->GetObjectClass(m_objHost);
		jmethodID printId = m_env->GetMethodID(
                objClass,"print", "(Ljava/lang/String;)V");
		jstring jstr = m_env->NewStringUTF(info.c_str());
		//jobject result = m_env->CallObjectMethod(m_objHost, printId, jstr);
		m_env->CallVoidMethod(m_objHost, printId, jstr);
		m_env->DeleteLocalRef(jstr);
		return true;
	}
}