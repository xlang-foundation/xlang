#include <jni.h>
#include <string>
#include "xhost_impl.h"
#include "Hosting.h"
#include "androidwrapper.h"
#include "object.h"

static X::Android::AndroidWrapper* _android = nullptr;
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
    X::XLangStaticLoad();
    jobject newRef_Host = env->NewGlobalRef(objHost);
    _android = new X::Android::AndroidWrapper(env, newRef_Host);
    X::RegisterPackage<X::Android::AndroidWrapper>("android",_android);
    return true;
};
extern "C" JNIEXPORT jlong JNICALL
Java_org_xlangfoundation_playground_xlang_loadModuleJNI(
        JNIEnv* env,
        jobject objHost,
        jstring codeObj) {
    std::string moduleName = "default";
    const char* szCode = env->GetStringUTFChars( codeObj, nullptr );
    unsigned long long moduleKey = 0;
    std::string code(szCode);
    auto* pModule = X::Hosting::I().Load(moduleName, code.c_str(), (int)code.size(), moduleKey);
    env->ReleaseStringUTFChars(codeObj,szCode);
    X::Value retVal;
    X::Hosting::I().InitRun(pModule,retVal);
    return (jlong)pModule;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_org_xlangfoundation_playground_xlang_runJNI(
        JNIEnv* env,
        jobject /* this */ activity,
        jstring codeObj) {
    std::string code = env->GetStringUTFChars( codeObj, nullptr );
    X::XLangStaticRun(code);
    return true;
};

extern "C" JNIEXPORT jboolean JNICALL
Java_org_xlangfoundation_playground_xlang_callJNI(
        JNIEnv* env,
        jobject /* this */ activity,
        jlong moduleKey,jlong callable,jobjectArray params) {
    X::AST::Module* pCurModule =(X::AST::Module*)moduleKey;
    X::Data::Object* pObj = dynamic_cast<X::Data::Object*>((X::XObj*)callable);
    X::ARGS args;
    X::KWARGS kwargs;
    X::Value retVal;
    pObj->Call(pCurModule->GetRT(), nullptr,args,kwargs,retVal);
    return true;
};

extern "C" JNIEXPORT jboolean JNICALL
Java_org_xlangfoundation_playground_xlang_unloadJNI(
        JNIEnv* env,
        jobject /* this */) {
    X::XLangStaticUnload();
    return true;
};