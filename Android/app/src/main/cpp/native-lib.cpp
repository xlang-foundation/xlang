#include <jni.h>
#include <string>
#include "xhost_impl.h"
#include "Hosting.h"
#include "androidwrapper.h"

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
    std::string code = env->GetStringUTFChars( codeObj, nullptr );
    unsigned long long moduleKey = 0;
    X::Hosting::I().Load(moduleName, code.c_str(), (int)code.size(), moduleKey);
    return moduleKey;
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
Java_org_xlangfoundation_playground_xlang_unloadJNI(
        JNIEnv* env,
        jobject /* this */) {
    X::XLangStaticUnload();
    return true;
};