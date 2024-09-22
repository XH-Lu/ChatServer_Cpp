#include "redis.hpp"
#include <iostream>
using namespace std;



Redis::Redis():_publishContext(nullptr), _subscribeContext(nullptr)
{}


Redis::~Redis()
{
    if(_publishContext != nullptr)
    {
        redisFree(_publishContext);
    }
    if(_subscribeContext != nullptr)
    {
        redisFree(_subscribeContext);
    }
}

bool Redis::connect()
{
    _publishContext = redisConnect("127.0.0.1", 6379);
    if(nullptr == _publishContext)
    {
        cerr << "connect redis failed!" << endl;
        return false;
    }

    _subscribeContext = redisConnect("127.0.0.1", 6379);
    if(nullptr == _subscribeContext)
    {
        cerr << "connect redis failed!" << endl;
        return false;
    }

    thread t(
        [&](){observerChannelMessage();}
    );
    t.detach();

    cout << "connect redis-server success!" << endl;
    return true;

}

bool Redis::publish(int channel, string msg)
{
    redisReply* reply = (redisReply*)redisCommand(_publishContext, "PUBLISH %d %s", channel, msg.c_str());
    if(nullptr == reply)
    {
        cerr << "publish command failed!" << endl;
        return false;
    }
    freeReplyObject(reply);
    return true;
}

bool Redis::subscribe(int channel)
{
    if(REDIS_ERR == redisAppendCommand(this->_subscribeContext, "SUBSCRIBE %d", channel))
    {
        cerr << "subscribe command failed!" << endl;
        return false;
    }
    int done = 0;
    while(!done)
    {
        if(REDIS_ERR == redisBufferWrite(this->_subscribeContext, &done))
        {
            cerr << "subscribe command failed!" <<endl;
            return false;
        }
    }

    return true;
}


bool Redis::unsubscribe(int channel)
{
    if(REDIS_ERR == redisAppendCommand(this->_subscribeContext, "UNSUBSCRIBE %d", channel))
    {
        cerr << "unsubscrive command failed!" << endl;
        return false;
    }

    int done = 0;
    while(!done)
    {
        if(REDIS_ERR == redisBufferWrite(this->_subscribeContext, &done))
        {
            cerr << "unscribe command failed!" << endl;
            return false;
        }
    }
    return true;
}

void Redis::observerChannelMessage()
{
    redisReply* reply = nullptr;
    while(REDIS_OK == redisGetReply(this->_subscribeContext, (void**)&reply))
    {
        if((reply != nullptr) && (reply->element[2] != nullptr) && (reply ->element[2]->str != nullptr))
        {
            _notifyMessageHandler(atoi(reply->element[1]->str), reply->element[2]->str);
        }
        freeReplyObject(reply);
    }
    cerr << ">>>>>>>>>> observerChannelMessage quit <<<<<<<<<<" << endl;
}

void Redis::initNotifyHandler(function<void(int,string)> fn)
{
    this->_notifyMessageHandler = fn;
}

