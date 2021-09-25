//
// Created by jeffli on 2021/9/7.
//

#include "audio_renderer.h"

#include <unistd.h>
#include "log.h"

#define ARRAY_LEN(a) (sizeof(a) / sizeof(a[0]))

#define RESULT_CHECK(result) \
    if (result != SL_RESULT_SUCCESS) { \
        return result; \
    } \

namespace media {

AudioRenderer::AudioRenderer()
  : sl_engine_(nullptr)
  , sl_engine_object_(nullptr)
  , output_mix_object_(nullptr)
  , player_object_(nullptr)
  , buffer_queue_(nullptr)
  , player_(nullptr)
  , player_status_(-1)
  , callback_(nullptr)
  , context_(nullptr)
  , position_(0) {

}

AudioRenderer::~AudioRenderer() = default;

SLresult AudioRenderer::Init(int channels, int sample_rate, ::media::AudioPlayerCallback callback, void *context) {
    LOGI("Enter: %s, channel=%d, sample_rate=%d", __func__ , channels, sample_rate);
    context_ = context;
    callback_ = callback;

    /// OpenSL ES for Android is designed to be thread-safe,
    /// so this option request will be ignored, but it will
    /// make the source code portable to other platforms.
    SLEngineOption engineOptions[] = {{(SLuint32) SL_ENGINEOPTION_THREADSAFE, (SLuint32) SL_BOOLEAN_TRUE}};

    /// Create the OpenSL ES engine object
    SLresult result = slCreateEngine(&sl_engine_object_, ARRAY_LEN(engineOptions), engineOptions, 0,
                                     nullptr,
                                     nullptr);

    RESULT_CHECK(result)
    result = (*sl_engine_object_)->Realize(sl_engine_object_, SL_BOOLEAN_FALSE);
    RESULT_CHECK(result)

    result = (*sl_engine_object_)->GetInterface(sl_engine_object_, SL_IID_ENGINE, &sl_engine_);
    RESULT_CHECK(result)

    if (sl_engine_ == nullptr) {
        LOGE("%s create opensl engine failed", __func__ );
        return -1;
    }

    result = (*sl_engine_)->CreateOutputMix(sl_engine_, &output_mix_object_, 0, nullptr, nullptr);
    RESULT_CHECK(result)

    result = (*output_mix_object_)->Realize(output_mix_object_, SL_BOOLEAN_FALSE);
    RESULT_CHECK(result)

    SLDataLocator_AndroidSimpleBufferQueue data_source_locator = { SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
                                                                   2
    };
    SLDataFormat_PCM data_source_format = {
            SL_DATAFORMAT_PCM,
            (SLuint32) channels,
            (SLuint32) OpenSLSampleRate(sample_rate),
            SL_PCMSAMPLEFORMAT_FIXED_16,
            SL_PCMSAMPLEFORMAT_FIXED_16,
            (SLuint32) GetChannelMask(channels),
            SL_BYTEORDER_LITTLEENDIAN
    };

    SLDataSource data_source = {
            &data_source_locator, &data_source_format
    };
    SLDataLocator_OutputMix data_sink_locator = {
            SL_DATALOCATOR_OUTPUTMIX,
            output_mix_object_
    };
    SLDataSink data_sink = {
            &data_sink_locator, nullptr
    };
    SLInterfaceID interface_ids[] = {
            SL_IID_BUFFERQUEUE, SL_IID_MUTESOLO, SL_IID_VOLUME, SL_IID_PLAYBACKRATE
    };
    SLboolean requiredInterfaces[] = {
            SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE
    };

    /// Create audio player object
    result = (*sl_engine_)->CreateAudioPlayer(sl_engine_, &player_object_, &data_source,
                                           &data_sink, ARRAY_LEN(interface_ids), interface_ids, requiredInterfaces);
    RESULT_CHECK(result)

    result = (*player_object_)->Realize(player_object_, SL_BOOLEAN_FALSE);
    RESULT_CHECK(result)

    result = (*player_object_)->GetInterface(player_object_, SL_IID_BUFFERQUEUE, &buffer_queue_);
    RESULT_CHECK(result)

    result = (*buffer_queue_)->RegisterCallback(buffer_queue_, AudioPlayerCallback, this);
    RESULT_CHECK(result)

    result = (*player_object_)->GetInterface(player_object_, SL_IID_PLAY, &player_);
    RESULT_CHECK(result)

    LOGI("Leave: %s", __func__);
    return 0;
}

SLresult AudioRenderer::Start() {
    if (player_ == nullptr) {
        LOGE("%s player object is null.", __func__);
        return -1;
    }
    if (IsPlaying()) {
        return SL_RESULT_SUCCESS;
    }
    if (player_object_ == nullptr) {
        LOGE("%s player object is nullptr.", __func__);
        return -2;
    }
    if (buffer_queue_ == nullptr) {
        LOGE("%s buffer queue is nullptr.", __func__);
        return -3;
    }
    LOGI("audio_renderer.cc Enter: %s", __func__);
    (*buffer_queue_)->Clear(buffer_queue_);
    auto result = (*player_)->SetPlayState(player_, SL_PLAYSTATE_PLAYING);
    RESULT_CHECK(result)
    player_status_ = SL_PLAYSTATE_PLAYING;
    ProducePacket();
    LOGI("audio_renderer.cc Leave: %s", __func__);
    return SL_RESULT_SUCCESS;
}

SLresult AudioRenderer::Pause() {
    if (player_ == nullptr) {
        LOGE("%s player object is null.", __func__);
        return -1;
    }
    LOGI("enter: %s", __func__);
    auto result = (*player_)->SetPlayState(player_, SL_PLAYSTATE_PAUSED);
    RESULT_CHECK(result)
    player_status_ = SL_PLAYSTATE_PAUSED;
    LOGI("leave: %s", __func__);
    return SL_RESULT_SUCCESS;
}

SLresult AudioRenderer::Stop() {
    if (player_ == nullptr || player_object_ == nullptr) {
        LOGE("%s player object is null.", __func__);
        return -1;
    }
    LOGI("enter: %s", __func__);
    auto result = (*player_)->SetPlayState(player_, SL_PLAYSTATE_STOPPED);
    RESULT_CHECK(result)
    player_status_ = SL_PLAYSTATE_STOPPED;
    usleep(10000);
    DestroyContext();
    LOGI("leave: %s", __func__);
    return SL_RESULT_SUCCESS;
}

bool AudioRenderer::IsPlaying() {
    bool result = false;
    SLuint32 pState = SL_PLAYSTATE_PLAYING;
    if (player_object_ != nullptr && player_ != nullptr) {
        (*player_)->GetPlayState(player_, &pState);
    } else {
        result = false;
    }
    if (pState == SL_PLAYSTATE_PLAYING) {
        result = true;
    }
    return result;
}

void AudioRenderer::DestroyContext() {
    LOGI("Enter: %s", __func__);
    if (player_object_ != nullptr) {
        (*player_object_)->Destroy(player_object_);
    }
    player_object_ = nullptr;
    if (output_mix_object_ != nullptr) {
        (*output_mix_object_)->Destroy(output_mix_object_);
    }
    output_mix_object_ = nullptr;
    if (sl_engine_object_ != nullptr) {
        (*sl_engine_object_)->Destroy(sl_engine_object_);
    }
    sl_engine_object_ = nullptr;
    buffer_queue_ = nullptr;
    LOGI("Leave: %s", __func__);
}

void AudioRenderer::AudioPlayerCallback(SLAndroidSimpleBufferQueueItf buffer_queue, void *context) {
    auto audio_renderer = reinterpret_cast<AudioRenderer*>(context);
    audio_renderer->ProducePacket();
}

void AudioRenderer::ProducePacket() {
    if (player_status_ == SL_PLAYSTATE_PLAYING) {
        uint8_t* buffer = nullptr;
        int buffer_size = 0;
        int ret = callback_(&buffer, &buffer_size, context_);
        if (ret == 0 && buffer_size > 0 && buffer != nullptr) {
            (*buffer_queue_)->Enqueue(buffer_queue_, buffer, buffer_size);
            (*player_)->GetPosition(player_, &position_);
        } else {
            LOGE("%s ret: %d buffer_size: %d", __func__, ret, buffer_size);
        }
    }
}

int AudioRenderer::OpenSLSampleRate(int sample_rate) {
    int result = SL_SAMPLINGRATE_44_1;
    switch (sample_rate) {
        case 8000:
            result = SL_SAMPLINGRATE_8;
            break;
        case 11025:
            result = SL_SAMPLINGRATE_11_025;
            break;
        case 12000:
            result = SL_SAMPLINGRATE_12;
            break;
        case 16000:
            result = SL_SAMPLINGRATE_16;
            break;
        case 22050:
            result = SL_SAMPLINGRATE_22_05;
            break;
        case 24000:
            result = SL_SAMPLINGRATE_24;
            break;
        case 32000:
            result = SL_SAMPLINGRATE_32;
            break;
        case 44100:
            result = SL_SAMPLINGRATE_44_1;
            break;
        case 48000:
            result = SL_SAMPLINGRATE_48;
            break;
        case 64000:
            result = SL_SAMPLINGRATE_64;
            break;
        case 88200:
            result = SL_SAMPLINGRATE_88_2;
            break;
        case 96000:
            result = SL_SAMPLINGRATE_96;
            break;
        case 192000:
            result = SL_SAMPLINGRATE_192;
            break;
        default:
            result = SL_SAMPLINGRATE_44_1;
    }
    return result;
}

int AudioRenderer::GetChannelMask(int channel) {
    int mask = SL_SPEAKER_FRONT_CENTER;
    switch (channel) {
        case 1:
            mask = SL_SPEAKER_FRONT_CENTER;
            break;
        case 2:
            mask = SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
            break;

        default:
            mask = SL_SPEAKER_FRONT_CENTER;
    }
    return mask;
}

}
