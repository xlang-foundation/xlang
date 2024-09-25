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

#include <jni.h>
#include <string>
#include "xhost_impl.h"
#include "Hosting.h"
#include "androidwrapper.h"
#include "object.h"

static X::Android::AndroidWrapper* _android = nullptr;
static JavaVM* jvm = 0;
namespace X
{
    extern void XLangStaticLoad();
    extern void XLangStaticRun(std::string code);
    extern void XLangStaticUnload();
}

extern "C" JNIEXPORT jboolean JNICALL
Java_org_xlangfoundation_playground_xlang_loadJNI(
        JNIEnv* env,
        jobject objHost) {
    if(jvm == 0)
    {
        env->GetJavaVM(&jvm);
    }
    _android = new X::Android::AndroidWrapper(env, objHost);
    _android->SetJVM(jvm);
    X::XLangStaticLoad();
    //X::RegisterPackage<X::Android::AndroidWrapper>("android",_android);
    X::RegisterPackage<X::Android::AndroidWrapper>("android","android",_android);
    _android->Init();
    _android->AddPlugins();
    return true;
};
extern "C" JNIEXPORT jlong JNICALL
Java_org_xlangfoundation_playground_xlang_loadModuleJNI(
        JNIEnv* env,
        jobject objHost,
        jstring codeObj) {
    //jvm->AttachCurrentThread(&env, NULL);
    //_android->SetHostPerThread(objHost);
    std::string moduleName = "default";
    const char* szCode = env->GetStringUTFChars( codeObj, nullptr );
    unsigned long long moduleKey = 0;
    std::string code(szCode);
    auto* pModule = X::Hosting::I().Load(moduleName.c_str(), code.c_str(), (int)code.size(), moduleKey);
    env->ReleaseStringUTFChars(codeObj,szCode);
    X::Value retVal;
    X::Hosting::I().InitRun(pModule,retVal);
    //jvm->DetachCurrentThread();
    return (jlong)pModule;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_org_xlangfoundation_playground_xlang_runJNI(
        JNIEnv* env,
        jobject objHost,
        jstring codeObj) {
    //_android->SetHostPerThread(objHost);
    std::string code = env->GetStringUTFChars( codeObj, nullptr );
    X::XLangStaticRun(code);
    return true;
};

extern "C" JNIEXPORT jboolean JNICALL
Java_org_xlangfoundation_playground_xlang_callJNI(
        JNIEnv* env,
        jobject objHost,
        jlong moduleKey,jlong callable,jobjectArray params) {
    //jvm->AttachCurrentThread(&env, NULL);
    //_android->SetHostPerThread(objHost);
    X::AST::Module* pCurModule =(X::AST::Module*)moduleKey;
    X::Data::Object* pObj = dynamic_cast<X::Data::Object*>((X::XObj*)callable);
    X::ARGS args;
    X::KWARGS kwargs;
    X::Value retVal;
    X::XRuntime* rt0 = (X::XRuntime*)pCurModule->GetRT();
    pObj->Call(rt0, nullptr,args,kwargs,retVal);
    //jvm->DetachCurrentThread();
    return true;
};

extern "C" JNIEXPORT jboolean JNICALL
Java_org_xlangfoundation_playground_xlang_callFromUIThreadJNI(
        JNIEnv* env,
        jobject objHost) {
    _android->CallFromUIThread();
    return true;
};

extern "C" JNIEXPORT jboolean JNICALL
Java_org_xlangfoundation_playground_xlang_unloadJNI(
        JNIEnv* env,
        jobject objHost) {
    //_android->SetHostPerThread(objHost);
    X::XLangStaticUnload();
    return true;
};