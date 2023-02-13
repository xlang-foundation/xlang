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
        UIBase::~UIBase()
        {
            AndroidWrapper* aw = m_page->GetApp()->GetParent();
            auto* env = aw->GetEnv();
            if(m_object)
            {
                env->DeleteLocalRef(m_object);
            }
        }
        bool UIElement::setText(std::string txt)
        {
            AndroidWrapper* aw = m_page->GetApp()->GetParent();
            auto* env = aw->GetEnv();
            auto* host = aw->GetHost();
            jclass objClass = env->GetObjectClass(host);
            jmethodID mId = env->GetMethodID(
                    objClass,"setText",
                    "(Ljava/lang/Object;Ljava/lang/String;)V");
            jstring jstr = env->NewStringUTF(txt.c_str());
            env->CallVoidMethod(host, mId,m_object,jstr);
            env->DeleteLocalRef(objClass);
            env->DeleteLocalRef(jstr);
            return true;
        }
        bool UIElement::setOnClickListener(X::Value handler)
        {
            AndroidWrapper* aw = m_page->GetApp()->GetParent();
            auto* env = aw->GetEnv();
            auto* host = aw->GetHost();
            jclass objClass = env->GetObjectClass(host);
            jmethodID mId = env->GetMethodID(
                    objClass,"setOnClickListener",
                    "(Ljava/lang/Object;J)V");
            handler.GetObj()->IncRef();
            long lHandler = (long)handler.GetObj();
            //TODO: add ref
            env->CallVoidMethod(host, mId,m_object,lHandler);
            env->DeleteLocalRef(objClass);
            return true;
        }
        bool ViewGroup::Add(UIBase* pElement)
        {
            m_kids.push_back(pElement);
            pElement->SetParent(this);
            AndroidWrapper* aw = m_page->GetApp()->GetParent();
            auto* env = aw->GetEnv();
            auto* host = aw->GetHost();
            jclass objClass = env->GetObjectClass(host);
            jmethodID mId = env->GetMethodID(
                    objClass,"addView",
                    "(Ljava/lang/Object;Ljava/lang/Object;)V");
            env->CallVoidMethod(host, mId,m_object,pElement->GetObject());
            env->DeleteLocalRef(objClass);

            return true;
        }
        LinearLayout::LinearLayout(Page* page):
            ViewGroup(page)
        {
            m_object = m_page->CreateLinearLayout();
        }
        TextView::TextView(Page* page):UIElement(page)
        {
            m_object = m_page->CreateTextView("text view");
        }
        Button::Button(Page* page):UIElement(page)
        {
            m_object = m_page->CreateButton("click me");
        }
        jobject Page::CreateTextView(std::string txt)
        {
            AndroidWrapper* aw = m_app->GetParent();
            auto* env = aw->GetEnv();
            auto* host = aw->GetHost();
            jclass objClass = env->GetObjectClass(host);
            jmethodID mId = env->GetMethodID(
                    objClass,"createTextview",
                    "(Ljava/lang/String;)Ljava/lang/Object;");
            jstring jstr = env->NewStringUTF(txt.c_str());
            jobject obj= env->CallObjectMethod(host, mId,jstr);
            env->DeleteLocalRef(objClass);
            env->DeleteLocalRef(jstr);
            return obj;
        }
        jobject Page::CreateButton(std::string txt) {
            AndroidWrapper* aw = m_app->GetParent();
            auto* env = aw->GetEnv();
            auto* host = aw->GetHost();
            jclass objClass = env->GetObjectClass(host);
            jmethodID mId = env->GetMethodID(
                    objClass,"createButton",
                    "(Ljava/lang/String;)Ljava/lang/Object;");
            jstring jstr = env->NewStringUTF(txt.c_str());
            jobject obj= env->CallObjectMethod(host, mId,jstr);
            env->DeleteLocalRef(objClass);
            env->DeleteLocalRef(jstr);
            return obj;
        }
        jobject Page::CreateLinearLayout()
        {
            AndroidWrapper* aw = m_app->GetParent();
            auto* env = aw->GetEnv();
            auto* host = aw->GetHost();
            jclass objClass = env->GetObjectClass(host);
            jmethodID mId = env->GetMethodID(
                    objClass,"createLinearLayout",
                    "()Ljava/lang/Object;");
            jobject obj= env->CallObjectMethod(host, mId);
            env->DeleteLocalRef(objClass);
            return obj;
        }

        bool Page::Create()
        {
            std::string info("default");
            AndroidWrapper* aw = m_app->GetParent();
            auto* env = aw->GetEnv();
            auto* host = aw->GetHost();
            jclass objClass = env->GetObjectClass(host);
            jmethodID mId = env->GetMethodID(
                    objClass,"createPage",
                    "(Ljava/lang/String;)Ljava/lang/Object;");
            jstring jstr = env->NewStringUTF(info.c_str());
            m_object = env->CallObjectMethod(host, mId,jstr);
            //env->CallVoidMethod(host, mId, jstr);
            env->DeleteLocalRef(objClass);
            env->DeleteLocalRef(jstr);
            return  true;

        }

        bool AndroidWrapper::Print(std::string info)
        {
            __android_log_print(ANDROID_LOG_INFO, "xlang", "%s", info.c_str());
#if 0
            jclass objClass = m_env->GetObjectClass(m_objHost);
            jmethodID printId = m_env->GetMethodID(
                    objClass,"print", "(Ljava/lang/String;)V");
            jstring jstr = m_env->NewStringUTF(info.c_str());
            //jobject result = m_env->CallObjectMethod(m_objHost, printId, jstr);
            m_env->CallVoidMethod(m_objHost, printId, jstr);
            m_env->DeleteLocalRef(jstr);
#endif
            return true;
        }
    }
}