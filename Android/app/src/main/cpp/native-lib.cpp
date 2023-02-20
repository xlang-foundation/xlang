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
    //if(jvm == 0)
    //{
    //    env->GetJavaVM(&jvm);
    //}
    //jclass txtV_cls = env->FindClass ( "TextView" );
    //jobject obj = env->NewObjectV ( clazz );

    _android = new X::Android::AndroidWrapper(env, objHost);
    X::XLangStaticLoad();
    X::RegisterPackage<X::Android::AndroidWrapper>("android",_android);
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
    auto* pModule = X::Hosting::I().Load(moduleName, code.c_str(), (int)code.size(), moduleKey);
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
    pObj->Call(pCurModule->GetRT(), nullptr,args,kwargs,retVal);
    //jvm->DetachCurrentThread();
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