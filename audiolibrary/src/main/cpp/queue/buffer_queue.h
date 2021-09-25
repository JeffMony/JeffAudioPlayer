//
// Created by jeffli on 2021/9/7.
//

#ifndef jeff_buffer_queue_h
#define jeff_buffer_queue_h

#include <stdint.h>
#include <string>
#include <pthread.h>

namespace media {

typedef struct AvBuffer {
    short* data;
    uint8_t* buffer;
    int size;
    float position;

    int64_t time;
    int width;
    int height;

    AvBuffer() {
        data = nullptr;
        buffer = nullptr;
        size = 0;
        position = -1;
        time = 0;
        width = 0;
        height = 0;
    }

    ~AvBuffer() {
        if (data != nullptr) {
            delete[] data;
        }
        data = nullptr;
        if (buffer != nullptr) {
            delete[] buffer;
        }
        buffer = nullptr;
        size = 0;
        position = -1;
        time = 0;
        width = 0;
        height = 0;
    }
} AvBuffer;

/**
 * 音频原始数据链表
 */
typedef struct AudioBufferList {
    AvBuffer* buffer;
    struct AudioBufferList* next;

    AudioBufferList() {
        buffer = nullptr;
        next = nullptr;
    }
} AudioBufferList;


class BufferQueue {

public:
    BufferQueue();

    virtual ~BufferQueue();

    void Init();

    void Flush();

    /**
     * 放入一帧解码后的数据
     * @param av_buffer
     * @return 0成功
     */
    int Put(AvBuffer* av_buffer);

    /**
     * 获取一帧的音频数据
     * @param av_buffer 获取音频数据
     * @param block     是否阻塞获取
     * @return 0成功
     */
    int Get(AvBuffer** av_buffer, bool block);

    /**
     * 获取queue大小
     * @return
     */
    int Size();

    /**
     * 停止
     */
    void Abort();

private:
    AudioBufferList* first_;
    AudioBufferList* last_;
    int packet_size_;
    bool abort_request_;
    pthread_mutex_t mutex_;
    pthread_cond_t cond_;
};

}

#endif //jeff_buffer_queue_h
