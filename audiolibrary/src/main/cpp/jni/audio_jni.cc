//
// Created by jeffli on 2021/9/7.
//

#include <jni.h>

extern "C" {
#include "log.h"
#include "audio_env.h"
#include "libavutil/avutil.h"
}

#include "audio_player.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef NELEM
#define NELEM(x) ((int) (sizeof(x) / sizeof((x)[0])))
#endif

#define AUDIO_PLAYER "com/jeffmony/audiolibrary/AudioPlayer"

#define FFMPEG_LOG_ENABLE 1
#define FFMPEG_LOG_TAG "Audio_lib_ffmpeg"

static jlong JNI_Audio_Player_create(JNIEnv* env, jobject object) {
    auto audio_player = new media::AudioPlayer(object);
    return reinterpret_cast<jlong>(audio_player);
}

static void JNI_Audio_Player_prepare(JNIEnv* env, jobject object, jlong id, jstring j_url) {
    if (id <= 0) {
        return;
    }
    auto audio_url = env->GetStringUTFChars(j_url, JNI_FALSE);
    auto audio_player = reinterpret_cast<media::AudioPlayer*>(id);
    audio_player->Prepare(audio_url);
    env->ReleaseStringUTFChars(j_url, audio_url);
}

static void JNI_Audio_Player_start(JNIEnv* env, jobject object, jlong id) {
    if (id <= 0) {
        return;
    }
    auto audio_player = reinterpret_cast<media::AudioPlayer*>(id);
    audio_player->Start();
}

static void JNI_Audio_Player_pause(JNIEnv* env, jobject object, jlong id) {
    if (id <= 0) {
        return;
    }
    auto audio_player = reinterpret_cast<media::AudioPlayer*>(id);
    audio_player->Pause();
}

static void JNI_Audio_Player_resume(JNIEnv* env, jobject object, jlong id) {
    if (id <= 0) {
        return;
    }
    auto audio_player = reinterpret_cast<media::AudioPlayer*>(id);
    audio_player->Resume();
}

static void JNI_Audio_Player_stop(JNIEnv* env, jobject object, jlong id) {
    if (id <= 0) {
        return;
    }
    auto audio_player = reinterpret_cast<media::AudioPlayer*>(id);
    audio_player->Stop();
}

static void JNI_Audio_Player_destroy(JNIEnv* env, jobject object, jlong id) {
    if (id <= 0) {
        return;
    }
    auto audio_player = reinterpret_cast<media::AudioPlayer*>(id);
    delete audio_player;
}

static void JNI_Audio_Player_seek(JNIEnv* env, jobject object, jlong id, jlong time) {
    if (id <= 0) {
        return;
    }
    auto audio_player = reinterpret_cast<media::AudioPlayer*>(id);
    audio_player->Seek(time);
}

static jlong JNI_Audio_Player_getDuration(JNIEnv* env, jobject object, jlong id) {
    if (id <= 0) {
        return 0;
    }
    auto audio_player = reinterpret_cast<media::AudioPlayer*>(id);
    return audio_player->GetDuration();
}

static jlong JNI_Audio_Player_getCurrentPosition(JNIEnv* env, jobject object, jlong id) {
    if (id <= 0) {
        return 0;
    }
    auto audio_player = reinterpret_cast<media::AudioPlayer*>(id);
    return audio_player->GetCurrentPosition();
}

static void JNI_Audio_Player_setLoop(JNIEnv* env, jobject object, jlong id, jboolean loop) {
    if (id <= 0) {
        return;
    }
    auto audio_player = reinterpret_cast<media::AudioPlayer*>(id);
    audio_player->SetLoop(loop);
}

static void JNI_Audio_Player_setVolumeLevel(JNIEnv* env, jobject object, jlong id, jint volume_level) {
    if (id <= 0) {
        return;
    }
    auto audio_player = reinterpret_cast<media::AudioPlayer*>(id);
    audio_player->SetVolumeLevel(volume_level);
}

static void JNI_Audio_Player_setSpeed(JNIEnv* env, jobject object, jlong id, jfloat speed) {
    if (id <= 0) {
        return;
    }
    auto audio_player = reinterpret_cast<media::AudioPlayer*>(id);
    audio_player->SetSpeed(speed);
}

#if FFMPEG_LOG_ENABLE
void log_callback(void *ptr, int level, const char *fmt, va_list vl) {
    //ffmpeg中level越大打样的log优先级越低
    if (level > av_log_get_level())
        return;
    int android_level = ANDROID_LOG_UNKNOWN;
    if (level > AV_LOG_DEBUG) {
        android_level = ANDROID_LOG_DEBUG;
    } else if (level > AV_LOG_VERBOSE) {
        android_level = ANDROID_LOG_VERBOSE;
    } else if (level > AV_LOG_INFO) {
        android_level = ANDROID_LOG_INFO;
    } else if (level > AV_LOG_WARNING) {
        android_level = ANDROID_LOG_WARN;
    } else if (level > AV_LOG_ERROR) {
        android_level = ANDROID_LOG_ERROR;
    } else if (level > AV_LOG_FATAL) {
        android_level = ANDROID_LOG_FATAL;
    }
    __android_log_vprint(android_level, FFMPEG_LOG_TAG, fmt, vl);
}
#endif

static JNINativeMethod audio_player_methods[] = {
        {"create",             "()J",                        (void **)JNI_Audio_Player_create },
        {"prepare",            "(JLjava/lang/String;)V",     (void **) JNI_Audio_Player_prepare },
        {"start",              "(J)V",                       (void **) JNI_Audio_Player_start },
        {"pause",              "(J)V",                       (void **) JNI_Audio_Player_pause },
        {"resume",             "(J)V",                       (void **) JNI_Audio_Player_resume },
        {"stop",               "(J)V",                       (void **) JNI_Audio_Player_stop },
        {"destroy",            "(J)V",                       (void **) JNI_Audio_Player_destroy },
        {"seek",               "(JJ)V",                      (void **) JNI_Audio_Player_seek },
        {"getDuration",        "(J)J",                       (void **) JNI_Audio_Player_getDuration },
        {"getCurrentPosition", "(J)J",                       (void **) JNI_Audio_Player_getCurrentPosition},
        {"setLoop",            "(JZ)V",                      (void **) JNI_Audio_Player_setLoop },
        {"setVolumeLevel",     "(JI)V",                      (void **) JNI_Audio_Player_setVolumeLevel },
        {"setSpeed",           "(JF)V",                      (void **) JNI_Audio_Player_setSpeed }
};


JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env = nullptr;
    if ((vm)->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }

    audio_jni_set_java_vm(vm);

    auto audio_player = env->FindClass(AUDIO_PLAYER);
    env->RegisterNatives(audio_player, audio_player_methods, NELEM(audio_player_methods));
    env->DeleteLocalRef(audio_player);

#if FFMPEG_LOG_ENABLE
    av_log_set_level(AV_LOG_VERBOSE);
    av_log_set_callback(log_callback);
#endif

    return JNI_VERSION_1_6;
}

#ifdef __cplusplus
}
#endif
