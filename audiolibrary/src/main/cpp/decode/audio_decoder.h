//
// Created by jeffli on 2021/9/7.
//

#ifndef jeff_audio_decoder_h
#define jeff_audio_decoder_h

#include <pthread.h>
#include <unistd.h>
#include "buffer_queue.h"
#include "common.h"

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswresample/swresample.h"
};

namespace media {

enum DecodeState {
    NORMAL,
    NEED_READ_FIRST,
    NEED_NEW_PACKET,
    FINISHED
};

class AudioDecoder {

public:
    AudioDecoder();
    virtual ~AudioDecoder();

    /**
     * 初始化ffpeg解码器
     * @param path 音频文件的地址
     * @return 0成功，其他失败
     */
    int Init(const char* path);

    /**
     * 释放内存
     */
    void Destroy();

    /**
     * 播放器拖动进度条
     * @param time
     */
    void Seek(int64_t time);

    /**
     * 获取音频数据
     * @param audio_data
     * @param size
     * @return
     */
    int GetFrameData(uint8_t** audio_data, int* size, int64_t* time);

    /**
     * 终止队列工作
     */
    void Abort();

    /**
     * 获取音频的时长
     * @return
     */
    long GetDuration();

    /**
     * 设置是否循环播放
     * @param loop
     */
    void SetLoop(bool loop);

private:

    static void* DecodeAudioThread(void* data);

    /**
     * 读取音频帧数据
     */
    void ReadPacketData();

    /**
     * 获取解码后的音频数据
     */
    void ReceiveFrameData();

    /**
     * 清空队列中音频数据
     * @param frame
     */
    void Flush();

    /**
     * 初始化重采样实例
     * @param format
     */
    void InitSwrContext(int format);

    /**
     * 释放重采样资源
     */
    void FreeSwrContext();

private:
    /// 解码线程
    pthread_t decode_thread_;

    /// format context 上下文
    AVFormatContext* format_context_;
    ///解码上下文
    AVCodecContext* codec_context_;
    /// 重采样上下文
    SwrContext* swr_context_;
    /// 解码之后的存放队列
    BufferQueue* buffer_queue_;
    /// 重采样的音频数据，8位
    uint8_t* resample_audio_buffer_;
    /// 音频时长，单位ms
    int64_t duration_;
    ///音频轨道的索引
    int audio_index_;
    ////停止解码
    bool abort_decoder_;
    /// 是否读到结尾
    bool is_read_end_;
    ///是否有seek请求
    bool is_seek_req_;
    ///播放器拖动的时间点
    int64_t seek_time_;
    ///设置是否循环播放
    bool loop_;

    /// 控制audio buffer queue的信号量
    pthread_mutex_t mutex_;
    pthread_cond_t cond_;
    pthread_mutex_t end_mutex_;
    pthread_cond_t end_cond_;

    DecodeState decode_state_;

};

}


#endif //jeff_audio_decoder_h
