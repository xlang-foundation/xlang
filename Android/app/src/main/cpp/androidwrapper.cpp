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

#include "androidwrapper.h"
#include <android/log.h>
#include "list.h"
#include "androidApi.x"



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

        bool UIThreadRun(X::Value& callable,void* pContext)
        {
            AndroidWrapper* aw =(AndroidWrapper*)pContext;
            aw->AddUICall(callable);
            return true;
        }
        bool AndroidWrapper::PostCallToJavaUIThread()
        {
            JNIEnv* env = nullptr;
            m_jvm->AttachCurrentThread(&env, nullptr);
            jclass objClass = env->GetObjectClass(m_objHost);
            jmethodID mid = env->GetMethodID(
                    objClass,"PostCallFromUIThread", "()V");
            env->CallVoidMethod(m_objHost, mid);
            env->DeleteLocalRef(objClass);
            m_jvm->DetachCurrentThread();
            return true;
        }
        void AndroidWrapper::CallFromUIThread()
        {
            while (true)
            {
                X::Value callable;
                m_uiCallLock.Lock();
                if (m_uiCalls.size() > 0)
                {
                    callable = m_uiCalls[0];
                    m_uiCalls.erase(m_uiCalls.begin());
                }
                m_uiCallLock.Unlock();
                if (callable.IsObject())
                {
                    X::Value retVal;
                    X::ARGS args;
                    X::KWARGS kwargs;
                    callable.GetObj()->Call(nullptr,nullptr,args, kwargs, retVal);
                }
                else
                {
                    break;
                }
            }
        }
        void AndroidWrapper::Init()
        {
            X::g_pXHost->RegisterUIThreadRunHandler(UIThreadRun,this);
        }
        void AndroidWrapper::AddPlugins()
        {
            APISET().GetPack()->RunCodeWithThisScope(Android_Plugin_Code);
        }
        Color::Color(X::ARGS& params, X::KWARGS& kwParams)
        {
            if(params.size() ==1 && params[0].IsObject())
            {
                auto& p0 = params[0];
                auto* pObj = p0.GetObj();
                if(pObj->GetType() == ObjType::Str)
                {
                    auto str_abi= pObj->ToString();
                    std::string name(str_abi);
                    X::g_pXHost->ReleaseString(str_abi);
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
        bool UIBase::setPadding(int left, int top, int right, int bottom)
        {
            AndroidWrapper* aw = m_page->GetApp()->GetParent();
            auto* env = aw->GetEnv();
            jclass objClass = env->GetObjectClass(m_object);
            jmethodID mId = env->GetMethodID(objClass,"setPadding","(IIII)V");
            env->CallVoidMethod(m_object, mId,left,top,right,bottom);
            env->DeleteLocalRef(objClass);
            return true;
        }
        bool UIBase::setGravity(int gravity)
        {
            AndroidWrapper* aw = m_page->GetApp()->GetParent();
            auto* env = aw->GetEnv();
            jclass objClass = env->GetObjectClass(m_object);
            jmethodID mId = env->GetMethodID(objClass,"setGravity","(I)V");
            env->CallVoidMethod(m_object, mId,gravity);
            env->DeleteLocalRef(objClass);
            return true;
        }
        bool UIBase::setMargins(int left, int top, int right, int bottom)
        {
            AndroidWrapper* aw = m_page->GetApp()->GetParent();
            auto* env = aw->GetEnv();
            auto* host = aw->GetHost();
            jclass objClass = env->GetObjectClass(host);
            jmethodID mId = env->GetMethodID(objClass,"setMargins","(Ljava/lang/Object;IIII)V");
            env->CallVoidMethod(host, mId,m_object,left,top,right,bottom);
            env->DeleteLocalRef(objClass);
            return true;
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
        ScrollView::ScrollView(Page* page):
                ViewGroup(page)
        {
            m_object = m_page->CreateScrollView();
        }
        TextView::TextView(Page* page):UIElement(page)
        {
            m_object = m_page->CreateTextView("text view");
        }
        EditText::EditText(Page* page):UIElement(page)
        {
            m_object = m_page->CreateEditText("text view");
        }
        Button::Button(Page* page):TextView(page)
        {
            m_object = m_page->CreateButton("");
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
        jobject Page::CreateScrollView()
        {
            AndroidWrapper* aw = m_app->GetParent();
            auto* env = aw->GetEnv();
            auto* host = aw->GetHost();
            jclass objClass = env->GetObjectClass(host);
            jmethodID mId = env->GetMethodID(
                    objClass,"createScrollview",
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
        bool App::ShowPage(Page* pPage)
        {
            AndroidWrapper* aw = GetParent();
            auto* env = aw->GetEnv();
            auto* host = aw->GetHost();
            jclass objClass = env->GetObjectClass(host);
            jmethodID mId = env->GetMethodID(
                    objClass,"showPage",
                    "(Ljava/lang/Object;)V");
            env->CallVoidMethod(host, mId,pPage->GetObject());
            env->DeleteLocalRef(objClass);
            return true;
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