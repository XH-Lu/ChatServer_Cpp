#include "redis.hpp"
#include <iostream>
using namespace std;
/***************************
数据层代码
主要任务：执行redis的连接、发布/订阅操作，以及接受订阅消息的回调
***************************/

Redis::Redis() : _publishContext(nullptr), _subscribeContext(nullptr)
{
}

Redis::~Redis()
{
    // 释放redisContext对象
    if (_publishContext != nullptr)
    {
        redisFree(_publishContext);
    }
    if (_subscribeContext != nullptr)
    {
        redisFree(_subscribeContext);
    }
}

bool Redis::connect()
{
    // 连接redis端口
    _publishContext = redisConnect("127.0.0.1", 6379);
    if (nullptr == _publishContext)
    {
        cerr << "connect redis failed!" << endl;
        return false;
    }

    _subscribeContext = redisConnect("127.0.0.1", 6379);
    if (nullptr == _subscribeContext)
    {
        cerr << "connect redis failed!" << endl;
        return false;
    }

    // 创建子线程进入观察者模式，等待订阅的频道收到消息
    thread t(
        [&]()
        { observerChannelMessage(); });
    t.detach();

    cout << "connect redis-server success!" << endl;
    return true;
}

bool Redis::publish(int channel, string msg)
{
    // 发布消息到channel，执行成功返回redisReply结构体，失败返回nullptr
    redisReply *reply = (redisReply *)redisCommand(_publishContext, "PUBLISH %d %s", channel, msg.c_str());
    if (nullptr == reply)
    {
        cerr << "publish command failed!" << endl;
        return false;
    }
    freeReplyObject(reply);
    return true;
}

bool Redis::subscribe(int channel)
{
    // 订阅channel通道消息
    if (REDIS_ERR == redisAppendCommand(this->_subscribeContext, "SUBSCRIBE %d", channel))
    {
        cerr << "subscribe command failed!" << endl;
        return false;
    }
    int done = 0;
    while (!done)
    {
        if (REDIS_ERR == redisBufferWrite(this->_subscribeContext, &done))
        {
            cerr << "subscribe command failed!" << endl;
            return false;
        }
    }

    return true;
}

bool Redis::unsubscribe(int channel)
{
    // 取消订阅channel通道消息
    if (REDIS_ERR == redisAppendCommand(this->_subscribeContext, "UNSUBSCRIBE %d", channel))
    {
        cerr << "unsubscrive command failed!" << endl;
        return false;
    }

    int done = 0;
    while (!done)
    {
        if (REDIS_ERR == redisBufferWrite(this->_subscribeContext, &done))
        {
            cerr << "unscribe command failed!" << endl;
            return false;
        }
    }
    return true;
}

void Redis::observerChannelMessage()
{
    redisReply *reply = nullptr;
    // 阻塞线程，等待redis服务器返回订阅消息
    while (REDIS_OK == redisGetReply(this->_subscribeContext, (void **)&reply))
    {
        if ((reply != nullptr) && (reply->element[2] != nullptr) && (reply->element[2]->str != nullptr))
        {
            // 将订阅消息发送至业务层
            _notifyMessageHandler(atoi(reply->element[1]->str), reply->element[2]->str);
        }
        freeReplyObject(reply);
    }
    cerr << ">>>>>>>>>> observerChannelMessage quit <<<<<<<<<<" << endl;
}

void Redis::initNotifyHandler(function<void(int, string)> fn)
{
    // 初始化订阅消息回调函数-->相应业务层代码
    this->_notifyMessageHandler = fn;
}
