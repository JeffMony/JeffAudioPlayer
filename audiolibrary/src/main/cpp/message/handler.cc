//
// Created by jeffli on 2021/9/7.
//

#include "handler.h"
#include "log.h"
#include "common.h"

namespace media {

Handler::Handler()
  : message_queue_thread_(0)
  , message_queue_(nullptr) {

}

Handler::~Handler() = default;

void Handler::InitMessageQueue(std::string &message_queue_name) {
    message_queue_ = new MessageQueue();
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_create(&message_queue_thread_, &attr, MessageQueueThread, this);
    pthread_setname_np(message_queue_thread_, message_queue_name.c_str());
}

void Handler::DestroyMessageQueue() {
    LOGE("enter: %s queue size: %d", __func__, GetQueueSize());
    // 发送退出消息线程的message
    Message message;
    message.what = MESSAGE_QUEUE_LOOP_QUIT_FLAG;
    SendMessage(&message);
    // 等待消息线程执行完毕
    pthread_join(message_queue_thread_, nullptr);
    // 释放message 对列
    message_queue_->Abort();
    SAFE_DELETE(message_queue_)
}

int Handler::SendMessage(Message *msg) {
    if (message_queue_ == nullptr) {
        return 0;
    }
    msg->handler = this;
    return message_queue_->EnqueueMessage(msg);
}

void Handler::RemoveMessage(int what) {
    if (message_queue_ == nullptr) {
        return;
    }
    message_queue_->Remove(what);
}

void Handler::FlushMessage() {
    if (message_queue_ == nullptr) {
        return;
    }
    message_queue_->Flush();
}

void Handler::SignalMessage() {
    message_queue_->Signal();
}

int Handler::GetQueueSize() {
    if (message_queue_ == nullptr) {
        return 0;
    }
    return message_queue_->Size();
}

void Handler::ProcessMessage() {
    bool running = true;
    while (running) {
        Message* msg = nullptr;
        if (message_queue_->DequeueMessage(&msg, true) > 0) {
            if (msg != nullptr) {
                if (MESSAGE_QUEUE_LOOP_QUIT_FLAG == msg->Execute()) {
                    LOGE("MESSAGE_QUEUE_LOOP_QUIT_FLAG");
                    running = false;
                }
            }
        }
    }
}

void * Handler::MessageQueueThread(void *context) {
    auto handler = reinterpret_cast<Handler*>(context);
    handler->ProcessMessage();
    pthread_exit(nullptr);
}

}
