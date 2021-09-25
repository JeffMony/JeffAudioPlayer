//
// Created by jeffli on 2021/9/7.
//

#ifndef jeff_handler_h
#define jeff_handler_h

#include <string>

#include "message_queue.h"

namespace media {

class Handler {

public:
    explicit Handler();
    virtual ~Handler();

    /**
     * 初始化消息队列
     * @param message_queue_name
     */
    void InitMessageQueue(std::string& message_queue_name);

    /**
     * 停止消息队列工作，并且销毁对应的资源
     */
    void DestroyMessageQueue();

    /**
     * 发送一个消息到消息队列中，等待被执行
     * @param msg
     * @return
     */
    int SendMessage(Message *msg);

    /**
     * 删除消息队列中对应what的所有消息
     * @param what
     */
    void RemoveMessage(int what);

    /**
     * 刷出当前队列中所有消息
     */
    void FlushMessage();

    /**
     * 唤醒消息队列
     */
    void SignalMessage();

    /**
     * 获取消息队列中还有多少未执行的消息
     * @return
     */
    int GetQueueSize();

    /**
     * 处理消息队里中的消息，需要子类继承
     * @param msg
     */
    virtual void HandleMessage(Message *msg) {}

private:
    void ProcessMessage();

    /**
     * 处理消息队列的线程
     * @param context
     * @return
     */
    static void* MessageQueueThread(void* context);

private:
    pthread_t message_queue_thread_;
    MessageQueue* message_queue_;
};

}


#endif //jeff_handler_h
