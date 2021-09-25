//
// Created by jeffli on 2021/9/7.
//

#ifndef jeff_audio_env_h
#define jeff_audio_env_h

#include <jni.h>

int audio_jni_set_java_vm(void* vm);

JavaVM* audio_jni_get_java_vm();

int jni_get_env(JNIEnv** env);

void jni_detach_thread_env();

#endif //jeff_audio_env_h
