//
// Created by jeffli on 2021/9/7.
//

#ifndef jeff_audio_player_h
#define jeff_audio_player_h

#include <jni.h>
#include <string>

#include "handler.h"
#include "audio_decoder.h"
#include "audio_renderer.h"

extern "C" {
#include "audio_env.h"
#include "sonic.h"
}

namespace media {

class AudioPlayer : public Handler {

public:
    AudioPlayer(jobject object);
    virtual ~AudioPlayer();

    void HandleMessage(Message *msg) override;

    void Prepare(const char* path);

    void Start();

    void Pause();

    void Resume();

    void Seek(int64_t time);

    void Stop();

    long GetDuration();

    long GetCurrentPosition();

    void SetLoop(bool loop);

    void SetVolumeLevel(int volume_level);

    void SetSpeed(float speed);

private:

    void PrepareInternal(const char* audio_path);

    void StartInternal();

    void PauseInternal();

    void ResumeInternal();

    void SeekInternal(int64_t time);

    void StopInternal();

    void SetLoopInternal(bool loop);

    void CompleteInternal();

    void OnError(int code);

    void OnPrepared();

    void OnCompleted();

    static int AudioDataCallback(uint8_t** buffer_data, int* buffer_size, void* context);

    int GetAudioFrameData();

    int ProcessNormalAudio();

    int ProcessSonicAudio();

    int ProcessSonicAudioInternal(uint8_t* audio_buffer, int audio_size);

private:
    jobject audio_player_object_;
    AudioDecoder* audio_decoder_;
    AudioRenderer* audio_renderer_;
    bool has_prepared_;
    bool is_running_;

    uint8_t* audio_buffer_;
    /// buffer_size_表示的是audio_buffer_中元素的个数
    int buffer_size_;
    int64_t current_time_;
    sonicStream audio_sonic_;
    int volume_level_;
    float speed_;
};

}


#endif //jeff_audio_player_h
