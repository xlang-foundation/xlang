#include <jni.h>
#include <string>
#include "xhost_impl.h"
#include "androidwrapper.h"

static X::AndroidWrapper* _android = nullptr;
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
    _android = new X::AndroidWrapper(env, newRef_Host);
    X::RegisterPackage<X::AndroidWrapper>("android",_android);
    return true;
};
extern "C" JNIEXPORT jlong JNICALL
Java_org_xlangfoundation_playground_xlang_loadModuleJNI(
        JNIEnv* env,
        jobject objHost,
        jstring codeObj) {
    return 0;
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