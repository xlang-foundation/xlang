#include "androidwrapper.h"
#include <android/log.h>

#if 0
X::Value::operator X::Android::UIBase* () const
{
    if (x.obj->GetType() == ObjType::Package)
    {
        XPackage* pPack = dynamic_cast<XPackage*>(x.obj);
        return (X::Android::UIBase*)pPack->GetEmbedObj();
    }
    else
    {
        return nullptr;
    }
}
#endif

/*
 Type     Chararacter
boolean      Z
byte         B
char         C
double       D
float        F
int          I
long         J
object       L
short        S
void         V
array        [
 */
namespace X
{
    namespace  Android
    {
        bool Page::Create()
        {
            std::string title("default");
            AndroidWrapper* aw = m_app->GetParent();
            auto* env = aw->GetEnv();
            auto* host = aw->GetHost();
            jclass objClass = env->GetObjectClass(host);
            jmethodID mId = env->GetMethodID(
                    objClass,"createPage", "(Ljava/lang/String;)Ljava/lang/Object;");
            jstring jstr = env->NewStringUTF(title.c_str());
            jobject result = env->CallObjectMethod(host, mId, jstr);
            //env->DeleteLocalRef(objClass);
            //env->DeleteLocalRef(result);
            env->DeleteLocalRef(jstr);
        }
        bool AndroidWrapper::Print(std::string info)
        {
            __android_log_print(ANDROID_LOG_INFO, "xlang", "%s", info.c_str());
            /*
            jclass objClass = m_env->GetObjectClass(m_objHost);
            jmethodID printId = m_env->GetMethodID(
                    objClass,"print", "(Ljava/lang/String;)V");
            jstring jstr = m_env->NewStringUTF(info.c_str());
            //jobject result = m_env->CallObjectMethod(m_objHost, printId, jstr);
            m_env->CallVoidMethod(m_objHost, printId, jstr);
            m_env->DeleteLocalRef(jstr);*/
            return true;
        }
    }
}