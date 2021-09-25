//
// Created by jeffli on 2021/9/7.
//

#include "audio_player.h"

#include "log.h"

#define FILE_NAME "audio_player.cc"

enum PlayerMessage {
    kPrepare,
    kStart,
    kPause,
    kResume,
    kSeek,
    kStop,
    kSetLoop,
    kCompleted,
};

namespace media {

AudioPlayer::AudioPlayer(jobject object)
  : audio_decoder_(nullptr)
  , has_prepared_(false)
  , is_running_(false)
  , buffer_size_(0)
  , current_time_(0)
  , audio_sonic_(nullptr)
  , volume_level_(100)
  , speed_(1.0f) {

    /// volume_level_默认情况是全部打开的
    LOGI("%s", __func__ );
    JNIEnv* env = nullptr;
    int ret = jni_get_env(&env);
    if (env != nullptr) {
        audio_player_object_ = env->NewGlobalRef(object);
    }
    if (ret == JNI_EDETACHED) {
        jni_detach_thread_env();
    }
    audio_renderer_ = new AudioRenderer();
    audio_sonic_ = sonicCreateStream(RESAMPLE_RATE, RESAMPLE_CHANNEL);
    audio_buffer_ = new uint8_t[AUDIO_BUFFER_LENGTH * 2];
    memset(audio_buffer_, 0, AUDIO_BUFFER_LENGTH * 2);

    std::string message_queue_name("Audio Player Message Queue");
    InitMessageQueue(message_queue_name);
}

AudioPlayer::~AudioPlayer() {
    LOGI("Enter: %s", __func__ );
    is_running_ = false;
    has_prepared_ = false;
    current_time_ = 0;
    volume_level_ = 100;
    speed_ = 1.0f;
    if (audio_decoder_ != nullptr) {
        audio_decoder_->Abort();
    }
    DestroyMessageQueue();
    if (audio_renderer_ != nullptr) {
        audio_renderer_->Stop();
    }
    SAFE_DELETE(audio_renderer_)
    if (audio_sonic_ != nullptr) {
        sonicDestroyStream(audio_sonic_);
        audio_sonic_ = nullptr;
    }
    SAFE_DELETE(audio_decoder_)
    SAFE_DELETE_ARRAY(audio_buffer_);
    JNIEnv* env = nullptr;
    int ret = jni_get_env(&env);
    if (env != nullptr && audio_player_object_ != nullptr) {
        env->DeleteGlobalRef(audio_player_object_);
    }
    if (ret == JNI_EDETACHED) {
        jni_detach_thread_env();
    }
    LOGI("Leave: %s", __func__ );
}

void AudioPlayer::HandleMessage(Message *msg) {
    Handler::HandleMessage(msg);
    auto what = msg->what;
    switch (what) {
        case kPrepare: {
            auto path = reinterpret_cast<char*>(msg->obj1);
            PrepareInternal(path);
            delete [] path;
            break;
        }
        case kStart: {
            StartInternal();
            break;
        }
        case kPause: {
            PauseInternal();
            break;
        }
        case kResume: {
            ResumeInternal();
            break;
        }
        case kSeek: {
            SeekInternal(msg->arg7);
            break;
        }
        case kStop: {
            StopInternal();
            break;
        }
        case kSetLoop: {
            SetLoopInternal(msg->arg8);
            break;
        }
        case kCompleted: {
            CompleteInternal();
            break;
        }
        default:
            break;
    }
}

void AudioPlayer::Prepare(const char* path) {
    LOGI("%s %s path=%s", FILE_NAME, __func__ , path);
    auto length = strlen(path) + 1;
    char* audio_path = new char[length];
    snprintf(audio_path, length, "%s%c", path, 0);
    Message message;
    message.what = kPrepare;
    message.obj1 = audio_path;
    SendMessage(&message);
}

void AudioPlayer::PrepareInternal(const char* audio_path) {
    if (has_prepared_) {
        return;
    }
    int ret = audio_renderer_->Init(RESAMPLE_CHANNEL, RESAMPLE_RATE, AudioDataCallback, this);
    if (ret != SL_RESULT_SUCCESS) {
        OnError(-1);
        return;
    }
    audio_decoder_ = new AudioDecoder();
    ret = audio_decoder_->Init(audio_path);
    if (ret != 0) {
        OnError(ret);
    } else {
        has_prepared_ = true;
        OnPrepared();
    }
}

void AudioPlayer::Start() {
    LOGI("%s %s", FILE_NAME, __func__ );
    Message message;
    message.what = kStart;
    SendMessage(&message);
}

void AudioPlayer::StartInternal() {
    if (is_running_) {
        return;
    }
    if (audio_renderer_ != nullptr) {
        audio_renderer_->Start();
        is_running_ = true;
    }
}

void AudioPlayer::Pause() {
    LOGI("%s %s", FILE_NAME, __func__ );
    Message message;
    message.what = kPause;
    SendMessage(&message);
}

void AudioPlayer::PauseInternal() {
    if (!is_running_) {
        return;;
    }
    if (audio_renderer_ != nullptr) {
        audio_renderer_->Pause();
        is_running_ = false;
    }
}

void AudioPlayer::Resume() {
    LOGI("%s %s", FILE_NAME, __func__ );
    Message message;
    message.what = kResume;
    SendMessage(&message);
}

void AudioPlayer::ResumeInternal() {
    if (is_running_) {
        return;
    }
    if (audio_renderer_ != nullptr) {
        audio_renderer_->Start();
        is_running_ = true;
    }
}

void AudioPlayer::Seek(int64_t time) {
    LOGI("%s %s time=%lld", FILE_NAME, __func__ , time);
    Message message;
    message.what = kSeek;
    message.arg7 = time;
    SendMessage(&message);
}

void AudioPlayer::SeekInternal(int64_t time) {
    if (!has_prepared_) {
        return;
    }
    if (audio_decoder_ != nullptr) {
        audio_decoder_->Seek(time);
    }
}

void AudioPlayer::Stop() {
    LOGI("%s %s", FILE_NAME, __func__ );
    is_running_ = false;
    has_prepared_ = false;
    current_time_ = 0;
    Message message;
    message.what = kStop;
    SendMessage(&message);
}

void AudioPlayer::StopInternal() {
    if (audio_decoder_ != nullptr) {
        audio_decoder_->Abort();
    }
    if (audio_renderer_ != nullptr) {
        audio_renderer_->Stop();
    }
}

long AudioPlayer::GetDuration() {
    if (audio_decoder_ != nullptr) {
        return audio_decoder_->GetDuration();
    }
    return 0;
}

long AudioPlayer::GetCurrentPosition() {
    return current_time_;
}

void AudioPlayer::SetLoop(bool loop) {
    LOGI("%s %s loop=%d", FILE_NAME, __func__ , loop);
    Message message;
    message.what = kSetLoop;
    message.arg8 = loop;
    SendMessage(&message);
}

void AudioPlayer::SetLoopInternal(bool loop) {
    if (audio_decoder_ != nullptr) {
        audio_decoder_->SetLoop(loop);
    }
}

void AudioPlayer::SetVolumeLevel(int volume_level) {
    LOGI("%s %s volume_level=%d", FILE_NAME, __func__ , volume_level);
    volume_level_ = volume_level;
}

void AudioPlayer::SetSpeed(float speed) {
    LOGI("%s %s speed=%f", FILE_NAME, __func__ , speed);
    speed_ = speed;
}

void AudioPlayer::CompleteInternal() {
    OnCompleted();
    Stop();
}

/**
 * 这是送入opensl es中准备播放渲染的音频原始数据
 *
 * 如果需要调整音频的数据可以在这儿调整
 *
 * 现在的音频已经是重采样之后的数据了，channel : 2  sample_rate : 44100 Hz
 *
 * @param buffer_data
 * @param buffer_size
 * @param context
 * @return
 */
int AudioPlayer::AudioDataCallback(uint8_t **buffer_data, int *buffer_size, void *context) {
    auto audio_player = reinterpret_cast<AudioPlayer*>(context);
    int ret = audio_player->GetAudioFrameData();
    if (ret != 0) {
        audio_player->buffer_size_ = 1024 * RESAMPLE_CHANNEL;
        memset(audio_player->audio_buffer_, 0, audio_player->buffer_size_ * UINT8_SIZE);
    }
    *buffer_data = audio_player->audio_buffer_;
    *buffer_size = audio_player->buffer_size_ * UINT8_SIZE;
    return 0;
}

int AudioPlayer::GetAudioFrameData() {
    if (!is_running_) {
        return -1;
    }
    if (audio_decoder_ == nullptr) {
        return -2;
    }

    if (volume_level_ == 100 && fabs(speed_ - 1.0f) < 0.001f) {
        return ProcessNormalAudio();
    } else {
        return ProcessSonicAudio();
    }
}

int AudioPlayer::ProcessNormalAudio() {
    uint8_t* audio_buffer = nullptr;
    int audio_size = 0;
    int64_t time = 0;

    auto ret = audio_decoder_->GetFrameData(&audio_buffer, &audio_size, &time);
    /// 这儿的 audio_size 就是audio_buffer 中数组元素个数

    if (ret == AVERROR_EOF) {
        OnCompleted();
        Stop();
        return -2;
    }
    if (audio_buffer == nullptr || audio_size <= 0) {
        return -3;
    }
    if (time > 0) {
        current_time_ = time;
    }
    buffer_size_ = audio_size;
    memcpy(audio_buffer_, audio_buffer, audio_size * UINT8_SIZE);
    delete [] audio_buffer;
    return 0;
}

int AudioPlayer::ProcessSonicAudio() {
    if (audio_sonic_ == nullptr) {
        /// 可能被销毁了
        return -1;
    }
    int sonic_max_required = sonicGetMaxRequired(audio_sonic_);
    int sonic_channel = sonicGetNumChannels(audio_sonic_);
    uint8_t* sonic_audio_buffer = new uint8_t[AUDIO_BUFFER_LENGTH];
    int sonic_buffer_size = 0;
    bool is_end = false;

    while (sonic_buffer_size <= sonic_max_required * sonic_channel * SHORT_SIZE) {
        uint8_t* audio_buffer = nullptr;
        int audio_size = 0;
        int64_t time = 0;
        int ret = audio_decoder_->GetFrameData(&audio_buffer, &audio_size, &time);
        if (ret == AVERROR_EOF) {
            is_end = true;
            break;
        }
        if (audio_buffer == nullptr || audio_size <= 0) {
            continue;
        }
        if (time > 0) {
            current_time_ = time;
        }
        memcpy(sonic_audio_buffer + sonic_buffer_size, audio_buffer, audio_size);
        sonic_buffer_size += audio_size;
    }

    if (is_end) {
        if (sonic_buffer_size > 0) {
            buffer_size_ = sonic_buffer_size;
            memcpy(audio_buffer_, sonic_audio_buffer, sonic_buffer_size);
            Message message;
            message.what = kCompleted;
            SendMessage(&message);
            return 0;
        } else {
            OnCompleted();
            Stop();
            return -2;
        }
    }
    int ret = ProcessSonicAudioInternal(sonic_audio_buffer, sonic_buffer_size);
    delete [] sonic_audio_buffer;
    return ret;
}

/**
 * 经过ProcessSonicAudioInternal 处理之后，audio_buffer_和audio_size_存储着操作完的数据
 * @param audio_buffer
 * @param audio_size
 */
int AudioPlayer::ProcessSonicAudioInternal(uint8_t* audio_buffer, int audio_size) {
        ///这儿使用sonic来操作音量
        sonicSetVolume(audio_sonic_, volume_level_ * 1.0f / 100);
        sonicSetSpeed(audio_sonic_, speed_);

        int sonic_channel = sonicGetNumChannels(audio_sonic_);

        /// numSamples不要设置错了，这儿比较容易出错
        /// 从uint8_t* 到 short* 结构，这中间的数据转化要把握清楚
        sonicWriteShortToStream(audio_sonic_, reinterpret_cast<short *>(audio_buffer),
                                audio_size / SHORT_SIZE / sonic_channel);

        /// 因为audio_size表示的是uint8_t 元素个数，要转化为short需要除以SHORT_SIZE
        auto result_samples = new short[AUDIO_BUFFER_LENGTH];

        int stream_offset;
        int stream_size = 0;
        auto stream_buffer = new short[AUDIO_UNIT_LENGTH * sonic_channel];

        while (true) {
            if (stream_size >= AUDIO_BUFFER_LENGTH) {
                /// 超过buffer size限制了，不能继续取数据了
                break;
            }
            stream_offset = sonicReadShortFromStream(audio_sonic_, stream_buffer, AUDIO_UNIT_LENGTH);
            if (stream_offset <= 0) {
                break;
            }

            /// stream_size 当前buffer 流大小
            /// stream_offset * sonic_channel 本次取得的大小
            /// following_stream_size 当前计算如果数据填充进result_samples ，填充后的size应该多大，这里注意，不能超过限制
            int following_stream_size = stream_size + stream_offset * sonic_channel;
            if (following_stream_size >= AUDIO_BUFFER_LENGTH) {
                memcpy(result_samples + stream_size, stream_buffer, (AUDIO_BUFFER_LENGTH - stream_size) * SHORT_SIZE);
                stream_size = AUDIO_BUFFER_LENGTH;
                break;
            }
            memcpy(result_samples + stream_size, stream_buffer, stream_offset * sonic_channel * SHORT_SIZE);
            /// stream_offset 是指处理完的stream_buffer数组中元素个数
            stream_size += stream_offset * sonic_channel;
        }
        delete [] stream_buffer;
        buffer_size_ = stream_size * SHORT_SIZE;
        /// 操作音量之后的size是不变的，但是其他情况下size不一定不变了，这一点还是要注意的。
        /// audio_buffer_ 要保证至少是result_samples的2倍，因为audio_buffer是uint8_t*   result_samples是short*
        memcpy(audio_buffer_, result_samples, stream_size * SHORT_SIZE);

        delete [] result_samples;
        return 0;
}

void AudioPlayer::OnError(int code) {
    JNIEnv* env = nullptr;
    int ret = jni_get_env(&env);
    if (env == nullptr) {
        return;
    }
    auto clazz = env->GetObjectClass(audio_player_object_);
    auto methodId = env->GetMethodID(clazz, "onError", "(ILjava/lang/String;)V");
    env->CallVoidMethod(audio_player_object_, methodId, code, nullptr);
    env->DeleteLocalRef(clazz);
    if (ret == JNI_EDETACHED) {
        jni_detach_thread_env();
    }
}

void AudioPlayer::OnPrepared() {
    JNIEnv* env = nullptr;
    int ret = jni_get_env(&env);
    if (env == nullptr) {
        return;
    }
    auto clazz = env->GetObjectClass(audio_player_object_);
    auto methodId = env->GetMethodID(clazz, "onPrepared", "()V");
    env->CallVoidMethod(audio_player_object_, methodId);
    env->DeleteLocalRef(clazz);
    if (ret == JNI_EDETACHED) {
        jni_detach_thread_env();
    }
}

void AudioPlayer::OnCompleted() {
    JNIEnv* env = nullptr;
    int ret = jni_get_env(&env);
    if (env == nullptr) {
        return;
    }
    auto clazz = env->GetObjectClass(audio_player_object_);
    auto methodId = env->GetMethodID(clazz, "onCompleted", "()V");
    env->CallVoidMethod(audio_player_object_, methodId);
    env->DeleteLocalRef(clazz);
    if (ret == JNI_EDETACHED) {
        jni_detach_thread_env();
    }
}

}