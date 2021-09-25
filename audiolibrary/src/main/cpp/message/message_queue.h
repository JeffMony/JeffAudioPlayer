//
// Created by jeffli on 2021/9/7.
//

#ifndef jeff_message_queue_h
#define jeff_message_queue_h

#include <pthread.h>

#define MESSAGE_QUEUE_LOOP_QUIT_FLAG  100000

namespace media {

class Handler;

class Message {

public:
    Message();
    ~Message();

    /**
     * 执行消息
     * @return
     */
    int Execute();

public:
    int what;
    int arg1;
    int arg2;
    int arg3;
    int arg4;
    int arg5;
    int arg6;
    int64_t arg7;
    bool arg8;
    void* obj1;
    void* obj2;

    Handler* handler;
    Message* next;

};

class MessageQueue {

public:
    MessageQueue();
    ~MessageQueue();

    /**
     * 刷出消息链表中的所有消息
     */
    void Flush();

    /**
     * 删除消息链表中特定的消息
     * @param what
     */
    void Remove(int what);

    /**
     * 唤醒消息队列
     */
    void Signal();

    /**
     * 添加消息到消息队列中
     * @param msg
     * @return 0成功，其他失败
     */
    int EnqueueMessage(Message* msg);

    /**
     * 从消息队列中取出一个消息
     * @param msg
     * @param block 是否阻塞等待
     * @return 0成功，其他失败
     */
    int DequeueMessage(Message** msg, bool block);

    /**
     * 消息队列中消息个数
     * @return
     */
    int Size();

    /**
     * 终止链表
     */
    void Abort();

private:
    Message* first_;
    Message* last_;
    int size_;
    bool abort_request_;

    pthread_mutex_t mutex_;
    pthread_cond_t cond_;

    Message* recycle_message_;
    int recycle_count_;
    int alloc_count_;
};

}

#endif //jeff_message_queue_h
