#include <jni.h>
#include <string>
#include "xload.h"

X::XLoad g_xLoad;
extern "C" JNIEXPORT jstring JNICALL
Java_org_xlangfoundation_xlang_MainActivity_loadJNI(
        JNIEnv* env,
        jobject /* this */) {
    auto* pXHost = X::CreatXHost();
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}