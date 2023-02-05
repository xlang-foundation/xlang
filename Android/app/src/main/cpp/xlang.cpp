#include <jni.h>
#include <string>
#include "xhost_impl.h"

namespace X
{
    extern void XLangStaticLoad();
    extern void XLangStaticRun(std::string code);
    extern void XLangStaticUnload();
}
extern "C" JNIEXPORT jboolean JNICALL
Java_org_xlangfoundation_xlang_MainActivity_loadJNI(
        JNIEnv* env,
        jobject /* this */) {
    X::XLangStaticLoad();
    return true;
};

extern "C" JNIEXPORT jboolean JNICALL
Java_org_xlangfoundation_xlang_MainActivity_runJNI(
        JNIEnv* env,
        jobject /* this */ activity,
        jstring codeObj) {
    std::string code = env->GetStringUTFChars( codeObj, nullptr );
    X::XLangStaticRun(code);
    return true;
};

extern "C" JNIEXPORT jboolean JNICALL
Java_org_xlangfoundation_xlang_MainActivity_unloadJNI(
        JNIEnv* env,
        jobject /* this */) {
    X::XLangStaticUnload();
    return true;
};