//
// Created by jeffli on 2021/9/7.
//

#include "buffer_queue.h"

#include "log.h"

namespace media {

BufferQueue::BufferQueue()
  : first_(nullptr)
  , last_(nullptr)
  , packet_size_(0)
  , abort_request_(false)
  , mutex_()
  , cond_() {
    Init();
}

BufferQueue::~BufferQueue() {
    Flush();
    pthread_mutex_destroy(&mutex_);
    pthread_cond_destroy(&cond_);
}

void BufferQueue::Init() {
    pthread_mutex_init(&mutex_, nullptr);
    pthread_cond_init(&cond_, nullptr);
}

void BufferQueue::Flush() {
    AudioBufferList* list;
    AudioBufferList* list1;
    pthread_mutex_lock(&mutex_);
    for (list = first_; list != nullptr; list = list1) {
        list1 = list->next;
        auto buffer = list->buffer;
        delete buffer;
        delete list;
    }
    last_ = nullptr;
    first_ = nullptr;
    packet_size_ = 0;
    pthread_mutex_unlock(&mutex_);
}

int BufferQueue::Put(AvBuffer *av_buffer) {
    if (abort_request_) {
        LOGE("%s audio packet queue abort.", __func__);
        delete av_buffer;
        return -1;
    }
    auto buffer_list = new AudioBufferList();
    buffer_list->buffer = av_buffer;
    buffer_list->next = nullptr;
    pthread_mutex_lock(&mutex_);
    if (last_ == nullptr) {
        first_ = buffer_list;
    } else {
        last_->next = buffer_list;
    }
    last_ = buffer_list;
    packet_size_++;
    pthread_cond_signal(&cond_);
    pthread_mutex_unlock(&mutex_);
    return 0;
}

int BufferQueue::Get(AvBuffer **av_buffer, bool block) {
    AudioBufferList* list;
    int ret;
    pthread_mutex_lock(&mutex_);
    for (;;) {
        if (abort_request_ && Size() <= 0) {
            ret = -1;
            break;
        }
        list = first_;
        if (list) {
            first_ = list->next;
            if (first_ == nullptr) {
                last_ = nullptr;
            }
            packet_size_--;
            *av_buffer = list->buffer;
            delete list;
            ret = 1;
            break;
        } else if (!block) {
            ret = 0;
            break;
        } else {
            pthread_cond_wait(&cond_, &mutex_);
        }
    }
    pthread_mutex_unlock(&mutex_);
    return ret;
}

int BufferQueue::Size() {
    pthread_mutex_lock(&mutex_);
    int size = packet_size_;
    pthread_mutex_unlock(&mutex_);
    return size;
}

void BufferQueue::Abort() {
    LOGI("Enter: %s queue: %p", __func__, this);
    pthread_mutex_lock(&mutex_);
    abort_request_ = true;
    pthread_cond_signal(&cond_);
    pthread_mutex_unlock(&mutex_);
    LOGI("Leave: %s", __func__);
}

}