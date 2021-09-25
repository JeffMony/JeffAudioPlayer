//
// Created by jeffli on 2021/9/7.
//

#include "audio_decoder.h"

#include "log.h"

#define AUDIO_QUEUE_MAX_SIZE 30
#define RESAMPLE_SIZE 1024

#define FILE_NAME "audio_decoder.cc"

namespace media {

AudioDecoder::AudioDecoder()
  : decode_thread_(0)
  , format_context_(nullptr)
  , codec_context_(nullptr)
  , swr_context_(nullptr)
  , duration_(0)
  , audio_index_(-1)
  , abort_decoder_(false)
  , is_read_end_(false)
  , is_seek_req_(false)
  , seek_time_(0)
  , loop_(false) {
    pthread_mutex_init(&mutex_, nullptr);
    pthread_cond_init(&cond_, nullptr);

    pthread_mutex_init(&end_mutex_, nullptr);
    pthread_cond_init(&end_cond_, nullptr);

    buffer_queue_ = new BufferQueue();
    resample_audio_buffer_ = new uint8_t[AUDIO_BUFFER_LENGTH];
    memset(resample_audio_buffer_, 0, AUDIO_BUFFER_LENGTH);
}

AudioDecoder::~AudioDecoder() {
    Destroy();
    SAFE_DELETE(buffer_queue_);
    SAFE_DELETE_ARRAY(resample_audio_buffer_);
    pthread_mutex_destroy(&mutex_);
    pthread_cond_destroy(&cond_);
    pthread_mutex_destroy(&end_mutex_);
    pthread_cond_destroy(&end_cond_);
}

int AudioDecoder::Init(const char *path) {
    LOGI("%s Enter: %s path=%s", FILE_NAME, __func__ , path);
    format_context_ = avformat_alloc_context();
    if (format_context_ == nullptr) {
        LOGE("%s avformat_alloc_context failed", __func__ );
        return -1;
    }
    int ret = avformat_open_input(&format_context_, path, nullptr, nullptr);
    if (ret < 0) {
        LOGE("%s avformat_open_input failed, err=%d, msg=%s", __func__ , ret, av_err2str(ret));
        return -2;
    }
    ret = avformat_find_stream_info(format_context_, nullptr);
    if (ret < 0) {
        LOGE("%s avformat_find_stream_info failed, err=%d, msg=%s", __func__ , ret, av_err2str(ret));
        return -3;
    }

    audio_index_ = av_find_best_stream(format_context_, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (audio_index_ == AVERROR_STREAM_NOT_FOUND) {
        LOGE("%s av_find_best_stream failed", __func__ );
        return -4;
    }
    AVStream* stream = format_context_->streams[audio_index_];
    AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);
    if (codec == nullptr) {
        LOGE("%s avcodec_find_decoder failed, codec_id=%d", __func__ , stream->codecpar->codec_id);
        return -5;
    }
    codec_context_ = avcodec_alloc_context3(codec);
    if (codec_context_ == nullptr) {
        LOGE("%s avcodec_alloc_context3 failed", __func__ );
        return -6;
    }
    ret = avcodec_parameters_to_context(codec_context_, stream->codecpar);
    if (ret < 0) {
        LOGE("%s avcodec_parameters_to_context failed, ret=%d, msg=%s",
             __func__ , ret, av_err2str(ret));
        return -7;
    }
    ret = avcodec_open2(codec_context_, codec, nullptr);
    if (ret < 0) {
        LOGE("%s avcodec_open2 failed, ret=%d, msg=%s", __func__ , ret, av_err2str(ret));
        return -8;
    }
    int channel = codec_context_->channels;
    int sample_rate = codec_context_->sample_rate;
    if (channel <= 0 || sample_rate <= 0) {
        LOGE("%s channel=%d, sample_rate=%d", __func__ , channel, sample_rate);
        return -9;
    }
    av_dump_format(format_context_, audio_index_, path, 0);
    duration_ = format_context_->duration / 1000;
    LOGI("%s channel=%d sample_rate=%d duration=%ld", __func__ , channel, sample_rate, duration_);
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_create(&decode_thread_, &attr, DecodeAudioThread, this);
    pthread_setname_np(decode_thread_, "audio_decode");
    LOGI("Leave %s", __func__ );
    return 0;
}

void * AudioDecoder::DecodeAudioThread(void *data) {
    auto decoder = reinterpret_cast<AudioDecoder*>(data);
    while (!decoder->abort_decoder_) {
        decoder->ReadPacketData();
    }
    pthread_exit(nullptr);
}

void AudioDecoder::ReadPacketData() {
    AVPacket *packet = av_packet_alloc();

    ///循环播放并且当前播放播放到结尾的时候，循环播放开始起作用了
    if (loop_ && is_read_end_) {
        /// 清空缓存中的数据
        Flush();

        int ret = av_seek_frame(format_context_, -1, 0, AVSEEK_FLAG_BACKWARD);
        if (ret < 0) {
            LOGE("%s %s ret=%d msg=%s", FILE_NAME, __func__ , ret, av_err2str(ret));
        }
        is_read_end_ = false;
    }

    if (is_seek_req_) {
        LOGI("%s %s seek_time=%ld", FILE_NAME, __func__ , seek_time_);
        Flush();
        int ret = av_seek_frame(format_context_, -1, seek_time_ * AV_TIME_BASE / 1000, AVSEEK_FLAG_BACKWARD);
        if (ret < 0) {
            LOGE("%s ret=%d, msg=%s", __func__ , ret, av_err2str(ret));
        }
        is_seek_req_ = false;
        is_read_end_ = false;
    }
    while (!abort_decoder_) {
        if (is_read_end_) {
            pthread_mutex_lock(&end_mutex_);
            pthread_cond_wait(&end_cond_, &end_mutex_);
            pthread_mutex_unlock(&end_mutex_);
        } else {
            int ret = av_read_frame(format_context_, packet);
            if (ret >= 0) {
                if (audio_index_ == packet->stream_index) {
                    ret = avcodec_send_packet(codec_context_, packet);
                    if (ret != 0) {
                        break;
                    }
                    if (buffer_queue_->Size() >= AUDIO_QUEUE_MAX_SIZE && !abort_decoder_) {
                        /// 音频队列满了，需要取出来了
                        pthread_mutex_lock(&mutex_);
                        pthread_cond_wait(&cond_, &mutex_);
                        pthread_mutex_unlock(&mutex_);
                    }
                    /// 发生拖动进度条，退出当前解码流程，重新seek到指定时间点开始解码
                    if (is_seek_req_) {
                        break;
                    }
                    ReceiveFrameData();
                    break;
                }
            } else {
                LOGE("%s %s, ret=%d, msg=%s",FILE_NAME, __func__ , ret, av_err2str(ret));
                is_read_end_ = (ret == AVERROR_EOF);

                /// 循环播放的时候也别忘了要将剩余的buffer queue中数据取出来
                if (loop_) {
                    /// 读到结束了，一定要将buffer_queue中数据取出来，不然播放的音频会少几十帧
                    while (buffer_queue_->Size() > 0) {
                        usleep(10);
                    }
                } else {
                    /// 读到结束了，一定要将buffer_queue中数据取出来，不然播放的音频会少几十帧
                    while (buffer_queue_->Size() > 0) {
                        usleep(10);
                    }
                    abort_decoder_ = true;
                    buffer_queue_->Abort();
                }
                break;
            }
        }
    }
    av_packet_free(&packet);

}

void AudioDecoder::ReceiveFrameData() {
    AVFrame* frame = av_frame_alloc();
    while (!abort_decoder_) {
        int out_samples = swr_context_ == nullptr ? 0 : swr_get_out_samples(swr_context_, 0);
        auto sample_size = RESAMPLE_SIZE;
        if (out_samples <= sample_size) {
            int ret = avcodec_receive_frame(codec_context_, frame);
            if (ret != 0) {
                break;
            }
            int64_t time_stamp = av_rescale_q(frame->pts, format_context_->streams[audio_index_]->time_base, AV_TIME_BASE_Q) / 1000;
            if (frame->channels <= 0 || frame->sample_rate <= 0) {
                continue;
            }
            InitSwrContext(frame->format);
            memset(resample_audio_buffer_, 0, AUDIO_BUFFER_LENGTH);
            int out_nb_samples = swr_convert(swr_context_, &resample_audio_buffer_, sample_size,
                                             (const uint8_t **) frame->data, frame->nb_samples);
            int line_size = 0;
            auto buffer_size = av_samples_get_buffer_size(&line_size, RESAMPLE_CHANNEL, out_nb_samples, AV_SAMPLE_FMT_S16, 1);
            auto audio_buffer = new AvBuffer();
            audio_buffer->buffer = new uint8_t[buffer_size];
            audio_buffer->size = buffer_size;
            audio_buffer->time = time_stamp;
            memcpy(audio_buffer->buffer, resample_audio_buffer_, buffer_size);
            buffer_queue_->Put(audio_buffer);
        } else {
            int out_nb_samples = swr_convert(swr_context_, &resample_audio_buffer_, sample_size, nullptr, 0);
            auto buffer_size = av_samples_get_buffer_size(nullptr, RESAMPLE_CHANNEL, out_nb_samples, AV_SAMPLE_FMT_S16, 1);
            auto audio_buffer = new AvBuffer();
            audio_buffer->buffer = new uint8_t[buffer_size];
            audio_buffer->size = buffer_size;
            audio_buffer->time = 0;
            memcpy(audio_buffer->buffer, resample_audio_buffer_, buffer_size);
            buffer_queue_->Put(audio_buffer);
        }
    }
    av_frame_free(&frame);
}

void AudioDecoder::Destroy() {
    LOGI("Enter: %s", __func__ );
    if (format_context_ == nullptr) {
        LOGI("%s format context is null.", __func__ );
        return;
    }
    abort_decoder_ = true;
    is_read_end_ = false;
    is_seek_req_ = false;
    loop_ = false;
    duration_ = 0;
    audio_index_ = -1;

    pthread_mutex_lock(&mutex_);
    pthread_cond_signal(&cond_);
    pthread_mutex_unlock(&mutex_);
    buffer_queue_->Abort();

    pthread_mutex_lock(&end_mutex_);
    pthread_cond_signal(&end_cond_);
    pthread_mutex_unlock(&end_mutex_);

    if (decode_thread_ != 0) {
        pthread_join(decode_thread_, nullptr);
    }
    if (format_context_ != nullptr) {
        avformat_close_input(&format_context_);
        format_context_ = nullptr;
    }
    if (codec_context_ != nullptr) {
        avcodec_free_context(&codec_context_);
        codec_context_ = nullptr;
    }
    FreeSwrContext();
    LOGI("Leave: %s", __func__ );
}

int AudioDecoder::GetFrameData(uint8_t **audio_data, int *size, int64_t* time) {
    if (is_read_end_) {
        if (buffer_queue_->Size() <= 0 && !loop_) {
            return AVERROR_EOF;
        }
        pthread_mutex_lock(&end_mutex_);
        pthread_cond_signal(&end_cond_);
        pthread_mutex_unlock(&end_mutex_);
    } else {
        if (buffer_queue_->Size() < AUDIO_QUEUE_MAX_SIZE) {
            pthread_mutex_lock(&mutex_);
            pthread_cond_signal(&cond_);
            pthread_mutex_unlock(&mutex_);
        }
    }

    AvBuffer* audio_buffer = nullptr;
    int ret = buffer_queue_->Get(&audio_buffer, true);
    if (ret < 0) {
        return ret;
    }
    if (audio_buffer == nullptr || audio_buffer->size <= 0) {
        delete audio_buffer;
        return -1;
    }
    *audio_data = audio_buffer->buffer;
    *size = audio_buffer->size;
    *time = audio_buffer->time;
    return ret;
}

void AudioDecoder::Seek(int64_t time) {
    LOGI("%s %s time=%lld", FILE_NAME, __func__ , time);
    is_seek_req_ = true;
    seek_time_ = time;

    pthread_mutex_lock(&mutex_);
    pthread_cond_signal(&cond_);
    pthread_mutex_unlock(&mutex_);

    pthread_mutex_lock(&end_mutex_);
    pthread_cond_signal(&end_cond_);
    pthread_mutex_unlock(&end_mutex_);
}

void AudioDecoder::Abort() {
    if (buffer_queue_ != nullptr) {
        buffer_queue_->Abort();
    }
}

long AudioDecoder::GetDuration() {
    return duration_;
}

void AudioDecoder::SetLoop(bool loop) {
    LOGI("%s %s %d", FILE_NAME, __func__ , loop);
    loop_ = loop;
}

void AudioDecoder::Flush() {
    AVFrame* frame = av_frame_alloc();
    avcodec_send_packet(codec_context_, nullptr);
    while(true) {
        int ret = avcodec_receive_frame(codec_context_, frame);
        if (ret == AVERROR_EOF) {
            break;
        }
    }
    avcodec_flush_buffers(codec_context_);
    buffer_queue_->Flush();
    av_frame_free(&frame);

    pthread_mutex_lock(&end_mutex_);
    pthread_cond_signal(&end_cond_);
    pthread_mutex_unlock(&end_mutex_);
}

void AudioDecoder::InitSwrContext(int format) {
    if (swr_context_ != nullptr) {
        return;
    }
    if (codec_context_ == nullptr) {
        return;
    }
    auto src_channel_layout = av_get_default_channel_layout(codec_context_->channels);
    swr_context_ = swr_alloc();
    if (swr_context_ == nullptr) {
        LOGE("%s swr context alloc failed", __func__ );
        return;
    }
    AVSampleFormat dest_sample_format = AV_SAMPLE_FMT_S16;
    auto dest_channel_layout = RESAMPLE_CHANNEL == 2 ? AV_CH_LAYOUT_STEREO : AV_CH_LAYOUT_MONO;
    swr_context_ = swr_alloc_set_opts(nullptr, dest_channel_layout, dest_sample_format, RESAMPLE_RATE,
                                      src_channel_layout, (AVSampleFormat) format, codec_context_->sample_rate,
                                      0, nullptr);
    if (swr_context_ == nullptr) {
        LOGE("%s swr_alloc_set_opts failed", __func__ );
        return;
    }
    int ret = swr_init(swr_context_);
    if (ret < 0) {
        LOGE("%s swr_init failed, err=%d, msg=%s", __func__ , ret, av_err2str(ret));
        FreeSwrContext();
    }
}

void AudioDecoder::FreeSwrContext() {
    if (swr_context_ != nullptr) {
        swr_free(&swr_context_);
        swr_context_ = nullptr;
    }
}

}