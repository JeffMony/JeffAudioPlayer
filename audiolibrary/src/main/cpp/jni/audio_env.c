//
// Created by jeffli on 2021/9/7.
//

#include "audio_env.h"

#include <pthread.h>

#include "log.h"

static JavaVM* java_vm;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_once_t once = PTHREAD_ONCE_INIT;

int audio_jni_set_java_vm(void* vm) {
    int ret = 0;
    pthread_mutex_lock(&lock);
    if (java_vm == NULL) {
        java_vm = vm;
    } else if (java_vm != vm) {
        ret = -1;
    }
    pthread_mutex_unlock(&lock);
    return ret;
}

JavaVM* audio_jni_get_java_vm() {
    void* vm;
    pthread_mutex_lock(&lock);
    vm = java_vm;
    pthread_mutex_unlock(&lock);
    return vm;
}

int jni_get_env(JNIEnv** env) {
    JavaVM* vm = audio_jni_get_java_vm();
    int ret = (*vm)->GetEnv(vm, (void **) env, JNI_VERSION_1_6);
    if (ret == JNI_EDETACHED) {
        if ((*vm)->AttachCurrentThread(vm, env, NULL) != JNI_OK) {
            LOGE("%s Failed to attach the JNI environment to the current thread", __func__);
            *env = NULL;
        }
    }
    return ret;
}

void jni_detach_thread_env() {
    JavaVM *vm = audio_jni_get_java_vm();
    (*vm)->DetachCurrentThread(vm);
}
