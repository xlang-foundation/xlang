#include "androidwrapper.h"
#include <android/log.h>
#include "list.h"

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
        Color::Color(X::ARGS& params, X::KWARGS& kwParams)
        {
            if(params.size() ==1 && params[0].IsObject())
            {
                auto& p0 = params[0];
                auto* pObj = p0.GetObj();
                if(pObj->GetType() == ObjType::Str)
                {
                    std::string name = pObj->ToString();
                    std::transform(name.begin(),
                                   name.end(), name.begin(),
                                   [](unsigned char c) { return std::toupper(c); });
                    _color = getColor(name);
                }
                else if(pObj->GetType() == ObjType::List)
                {
                    float R =0;
                    float G =0;
                    float B =0;
                    float A =255;
                    auto* pList = dynamic_cast<X::Data::List*>(pObj);
                    if(pList->Size()>=3)
                    {
                        R = (float)pList->Get(0);
                        G = (float)pList->Get(1);
                        B = (float)pList->Get(2);
                    }
                    if(pList->Size()>=3)
                    {
                        A = (float)pList->Get(3);
                    }
                    _color = getColor(R,G,B,A);
                }
                else if(pObj->GetType() == ObjType::Dict)
                {

                }
            }
            else if(params.size()>=3)
            {
                float R = (float)params[0];
                float G = (float)params[1];
                float B = (float)params[2];
                float A =255;
                if(params.size()>=4)
                {
                    A = (float)params[3];
                }
                _color = getColor(R,G,B,A);
            }
        }
        UIBase::~UIBase()
        {
            AndroidWrapper* aw = m_page->GetApp()->GetParent();
            auto* env = aw->GetEnv();
            if(m_object)
            {
                env->DeleteGlobalRef(m_object);
            }
        }
        std::string TextView::getText()
        {
            AndroidWrapper* aw = m_page->GetApp()->GetParent();
            auto* env = aw->GetEnv();
            auto* host = aw->GetHost();
            jclass objClass = env->GetObjectClass(host);
            jmethodID mId = env->GetMethodID(
                    objClass,"getText",
                    "(Ljava/lang/Object;)Ljava/lang/String;");
            jstring retObj= (jstring)env->CallObjectMethod(host, mId,m_object);
            const char* szStr = env->GetStringUTFChars( retObj, nullptr );
            std::string retStr(szStr);
            env->ReleaseStringUTFChars(retObj,szStr);
            env->DeleteLocalRef(objClass);
            env->DeleteLocalRef(retObj);
            return retStr;
        }
        bool TextView::setTextColor(Color* objColor)
        {
            AndroidWrapper* aw = m_page->GetApp()->GetParent();
            auto* env = aw->GetEnv();
            auto* host = aw->GetHost();
            jclass objClass = env->GetObjectClass(host);
            jmethodID mId = env->GetMethodID(
                    objClass,"setTextColor",
                    "(Ljava/lang/Object;I)V");
            env->CallVoidMethod(host, mId,m_object,objColor->getColor());
            env->DeleteLocalRef(objClass);
            return true;
        }
        bool UIBase::setBackgroundColor(Color* objColor)
        {

            AndroidWrapper* aw = m_page->GetApp()->GetParent();
            auto* env = aw->GetEnv();
            auto* host = aw->GetHost();
            jclass objClass = env->GetObjectClass(host);
            jmethodID mId = env->GetMethodID(
                    objClass,"setBackgroundColor",
                    "(Ljava/lang/Object;I)V");
            env->CallVoidMethod(host, mId,m_object,objColor->getColor());
            env->DeleteLocalRef(objClass);
            return true;
        }
        bool TextView::setText(std::string txt)
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
        bool TextView::setTextSize(int unit,float size)
        {
            AndroidWrapper* aw = m_page->GetApp()->GetParent();
            auto* env = aw->GetEnv();
            auto* host = aw->GetHost();
            jclass objClass = env->GetObjectClass(host);
            jmethodID mId = env->GetMethodID(
                    objClass,"setTextSize",
                    "(Ljava/lang/Object;IF)V");
            env->CallVoidMethod(host, mId,m_object,unit,size);
            env->DeleteLocalRef(objClass);
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
        EditText::EditText(Page* page):UIElement(page)
        {
            m_object = m_page->CreateEditText("text view");
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
            jobject objRef = env->NewGlobalRef(obj);
            env->DeleteLocalRef(objClass);
            env->DeleteLocalRef(jstr);
            return objRef;
        }
        jobject Page::CreateEditText(std::string txt)
        {
            AndroidWrapper* aw = m_app->GetParent();
            auto* env = aw->GetEnv();
            auto* host = aw->GetHost();
            jclass objClass = env->GetObjectClass(host);
            jmethodID mId = env->GetMethodID(
                    objClass,"createEditText",
                    "(Ljava/lang/String;)Ljava/lang/Object;");
            jstring jstr = env->NewStringUTF(txt.c_str());
            jobject obj= env->CallObjectMethod(host, mId,jstr);
            jobject objRef = env->NewGlobalRef(obj);
            env->DeleteLocalRef(objClass);
            env->DeleteLocalRef(jstr);
            return objRef;
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
            jobject objRef = env->NewGlobalRef(obj);
            env->DeleteLocalRef(objClass);
            env->DeleteLocalRef(jstr);
            return objRef;
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
            jobject objRef = env->NewGlobalRef(obj);
            env->DeleteLocalRef(objClass);
            return objRef;
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
            jobject obj = env->CallObjectMethod(host, mId,jstr);
            m_object = env->NewGlobalRef(obj);
            //env->CallVoidMethod(host, mId, jstr);
            env->DeleteLocalRef(objClass);
            env->DeleteLocalRef(jstr);
            return  true;

        }

        bool AndroidWrapper::Print(std::string info)
        {
            __android_log_print(ANDROID_LOG_INFO, "xlang", "%s", info.c_str());
#if 1
            jclass objClass = m_env->GetObjectClass(m_objHost);
            jmethodID printId = m_env->GetMethodID(
                    objClass,"print", "(Ljava/lang/String;)V");
            jstring jstr = m_env->NewStringUTF(info.c_str());
            //jobject result = m_env->CallObjectMethod(m_objHost, printId, jstr);
            m_env->CallVoidMethod(m_objHost, printId, jstr);
            m_env->DeleteLocalRef(jstr);
            m_env->DeleteLocalRef(objClass);
#endif
            return true;
        }
    }
}