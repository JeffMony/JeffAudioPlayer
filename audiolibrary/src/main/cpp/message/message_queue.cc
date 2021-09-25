//
// Created by jeffli on 2021/9/7.
//

#include "message_queue.h"
#include "handler.h"

namespace media {

Message::Message() :
handler(nullptr),
what(-1),
arg1(-1),
arg2(-1),
arg3(-1),
arg4(-1),
arg5(-1),
arg6(-1),
arg7(-1),
arg8(false),
obj1(nullptr),
obj2(nullptr) {

}

Message::~Message() = default;

int Message::Execute() {
    if (MESSAGE_QUEUE_LOOP_QUIT_FLAG == what) {
        return MESSAGE_QUEUE_LOOP_QUIT_FLAG;
    } else if (handler) {
        handler->HandleMessage(this);
        return 1;
    }
    return 0;
}


MessageQueue::MessageQueue() :
first_(nullptr),
last_(nullptr),
size_(0),
abort_request_(false),
mutex_(),
cond_(),
alloc_count_(0),
recycle_count_(0),
recycle_message_(nullptr) {
    pthread_mutex_init(&mutex_, nullptr);
    pthread_cond_init(&cond_, nullptr);
}

MessageQueue::~MessageQueue() {
    while (recycle_message_) {
        auto msg = recycle_message_;
        if (msg) {
            recycle_message_ = msg->next;
            delete msg;
        }
    }
    pthread_mutex_destroy(&mutex_);
    pthread_cond_destroy(&cond_);
}

void MessageQueue::Flush() {
    pthread_mutex_lock(&mutex_);
    Message* msg;
    Message* msg1;
    for (msg = first_; msg != nullptr; msg = msg1) {
        msg1 = msg->next;
        msg->next = recycle_message_;
        recycle_message_ = msg;
    }
    last_ = nullptr;
    first_ = nullptr;
    size_ = 0;
    pthread_mutex_unlock(&mutex_);
}

void MessageQueue::Remove(int what) {
    Message *msg, *last_msg;
    pthread_mutex_lock(&mutex_);
    last_msg = first_;
    if (!abort_request_ && first_) {
        auto p_msg = &first_;
        while (*p_msg) {
            msg = *p_msg;
            if (msg->what == what) {
                *p_msg = msg->next;
                msg->next = recycle_message_;
                recycle_message_ = msg;
                size_--;
            } else {
                last_msg = msg;
                p_msg = &msg->next;
            }
        }
        if (first_) {
            last_ = last_msg;
        } else {
            last_ = nullptr;
        }
    }
    pthread_mutex_unlock(&mutex_);
}

void MessageQueue::Signal() {
    pthread_mutex_lock(&mutex_);
    pthread_cond_signal(&cond_);
    pthread_mutex_unlock(&mutex_);
}

int MessageQueue::EnqueueMessage(Message *msg) {
    if (abort_request_) {
        return -1;
    }
    pthread_mutex_lock(&mutex_);
    Message* msg1 = recycle_message_;
    if (msg1) {
        recycle_message_ = msg1->next;
        recycle_count_++;
    } else {
        alloc_count_++;
        msg1 = new Message();
    }
    *msg1 = *msg;
    msg1->next = nullptr;
    if (!last_) {
        first_ = msg1;
    } else {
        last_->next = msg1;
    }
    last_ = msg1;
    size_++;
    pthread_cond_signal(&cond_);
    pthread_mutex_unlock(&mutex_);
    return 0;
}

int MessageQueue::DequeueMessage(Message **msg, bool block) {
    Message* msg1;
    int ret;
    pthread_mutex_lock(&mutex_);
    for (;;) {
        if (abort_request_) {
            ret = -1;
            break;
        }
        msg1 = first_;
        if (msg1) {
            first_ = msg1->next;
            if (!first_) {
                last_ = nullptr;
            }
            size_--;
            *msg = msg1;
            msg1->next = recycle_message_;
            recycle_message_ = msg1;
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

int MessageQueue::Size() {
    pthread_mutex_lock(&mutex_);
    auto size = size_;
    pthread_mutex_unlock(&mutex_);
    return size;
}

void MessageQueue::Abort() {
    pthread_mutex_lock(&mutex_);
    abort_request_ = true;
    pthread_cond_signal(&cond_);
    pthread_mutex_unlock(&mutex_);
}

}