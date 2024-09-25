#ifndef REDIS_H
#define REDIS_H

#include <hiredis/hiredis.h>
#include <thread>
#include <functional>
using namespace std;

class Redis
{
private:
    // 发布操作
    redisContext *_publishContext;
    // 订阅操作
    redisContext *_subscribeContext;
    // 接收消息回调函数
    function<void(int, string)> _notifyMessageHandler;

public:
    Redis();
    ~Redis();
    // 连接redis
    bool connect();
    // 发布消息
    bool publish(int channel, string message);
    // 订阅通道
    bool subscribe(int channel);
    // 取消订阅
    bool unsubscribe(int channel);
    // 监听redis服务器返回订阅消息
    void observerChannelMessage();
    // 初始化接收消息回调函数
    void initNotifyHandler(function<void(int, string)> fn);
};

#endif