#include <jni.h>
#include <string>

#include "libavutil/ffversion.h"

#include "mobileffmpeg.h"

JNIEXPORT jstring JNICALL Java_com_github_tanersener_mobileffmpeg_FFmpeg_getFFmpegVersion(JNIEnv* env, jobject object) {
    return env->NewStringUTF(FFMPEG_VERSION);
}

JNIEXPORT jstring JNICALL Java_com_github_tanersener_mobileffmpeg_FFmpeg_getVersion(JNIEnv* env, jobject object) {
    return env->NewStringUTF(MOBILE_FFMPEG_VERSION);
}

JNIEXPORT jstring JNICALL Java_com_github_tanersener_mobileffmpeg_FFmpeg_getABI(JNIEnv* env, jobject object) {
    #if defined(__arm__)
        #if defined(__ARM_ARCH_7A__)
            #if defined(__ARM_NEON__)
                return env->NewStringUTF(ABI_ARMV7A_NEON);
            #else
                return env->NewStringUTF(ABI_ARMV7A);
            #endif
        #else
            return env->NewStringUTF(ABI_ARM);
        #endif
    #elif defined(__i386__)
        return env->NewStringUTF(ABI_X86);
    #elif defined(__x86_64__)
        return env->NewStringUTF(ABI_X86_64);
    #elif defined(__aarch64__)
        return env->NewStringUTF(ABI_ARM64_V8A);
    #else
        return env->NewStringUTF(ABI_UNKNOWN);
    #endif
}

JNIEXPORT jstring JNICALL Java_com_github_tanersener_mobileffmpeg_FFmpeg_execute(JNIEnv* env, jobject object, jobjectArray stringArray) {
    int stringCount = env->GetArrayLength(stringArray);
    std::string argv("");

    for (int i=0; i<stringCount; i++) {
        jstring string = (jstring) (env->GetObjectArrayElement(stringArray, i));
        const char* rawString = env->GetStringUTFChars(string, 0);

        if (i != 0) {
            argv.append(" ");
        }
        argv.append(rawString);

        env->ReleaseStringUTFChars(string, rawString);
        env->DeleteLocalRef(string);
    }

    return env->NewStringUTF(argv.c_str());
}
