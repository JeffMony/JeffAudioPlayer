//
// Created by jeffli on 2021/9/7.
//

#ifndef jeff_audio_renderer_h
#define jeff_audio_renderer_h

#include <stdlib.h>

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

namespace media {

using AudioPlayerCallback = int(*)(uint8_t**, int*, void*);

class AudioRenderer {

public:
    AudioRenderer();
    virtual ~AudioRenderer();

    /**
     * 初始化OpenSL
     * @param channels 播放的声道数
     * @param sample_rate 播放的采样率
     * @param callback 给OpenSL pcm数据的回调
     * @param context  callback的上下文
     * @return SL_RESULT_SUCCESS 成功, 其它失败
    */
    SLresult Init(int channels, int sample_rate, AudioPlayerCallback callback, void* context);

    /**
     * 开始播放, OpenSL会循环从callback中获取pcm数据播放
     * @return SL_RESULT_SUCCESS 成功, 其它失败
    */
    SLresult Start();

    /**
     * 暂停播放
     * @return
     */
    SLresult Pause();

    /**
     * 停止播放, 回收OpenSL相关资源
     * @return
     */
    SLresult Stop();

    /**
     * 获取OpenSL播放状态
     * @return true播放中
    */
    bool IsPlaying();

    /**
     * 释放OpenSL的上下文
    */
    void DestroyContext();

private:
    static void AudioPlayerCallback(SLAndroidSimpleBufferQueueItf buffer_queue, void* context);

    void ProducePacket();

    static int OpenSLSampleRate(int sample_rate);

    static int GetChannelMask(int channel);

private:
    SLEngineItf sl_engine_;
    SLObjectItf sl_engine_object_;

    SLObjectItf output_mix_object_;
    SLObjectItf player_object_;
    SLAndroidSimpleBufferQueueItf buffer_queue_;
    SLPlayItf player_;
    int player_status_;
    ::media::AudioPlayerCallback callback_;
    void* context_;
    SLmicrosecond position_;

};

}

#endif //jeff_audio_renderer_h
